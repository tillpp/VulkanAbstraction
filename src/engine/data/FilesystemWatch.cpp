
#include "engine/data/FilesystemWatch.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <stop_token>
#include <vector>


#ifdef __linux__
#include <sys/select.h>
#include <sys/inotify.h>
#include <unistd.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#elif WIN32
#include <windows.h>
#include <iostream>

#define EVENT_SIZE  ( sizeof (struct _FILE_NOTIFY_INFORMATION) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#endif


FilesystemWatch::FilesystemWatch(std::filesystem::path path){
    //thread = std::jthread(&FilesystemWatch::run,this,std::filesystem::path(path));
    thread = std::jthread([this,path](std::stop_token token){
        this->run(token, path);
    });
}

void FilesystemWatch::run(std::stop_token token,std::filesystem::path path){
#ifdef __linux__
    fd = inotify_init();
    if ( fd < 0 ) {
        perror( "inotify_init" );
    }

    int length;
    char buffer[BUF_LEN];

    struct Folder{
        std::filesystem::path path;
        std::map<std::string,WatchDescriptor> children;
    };
    std::map<WatchDescriptor, Folder> folder;

    //recursivly add
    std::function<void(WatchDescriptor parentWd,std::filesystem::path path)> addChildren;
    addChildren = [&](WatchDescriptor parentWd,std::filesystem::path path){
        for (auto& dir: std::filesystem::directory_iterator(path)) {
            if(!dir.is_directory() || !dir.exists())
                continue;
            int wd = inotify_add_watch(fd, dir.path().c_str(), IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE);
            folder[wd] = {.path = dir.path()};
            folder[parentWd].children[dir.path().filename()] = wd;
            addChildren(wd,dir.path());
        }
    };
    std::function<std::filesystem::path(WatchDescriptor wd,std::string name)> registerPath = [&](WatchDescriptor parentWd,std::string name)->std::filesystem::path{
        assert(folder.find(parentWd) != folder.end());
        auto path = folder[parentWd].path/name;

        int wd = inotify_add_watch(fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE);
        folder[wd] = {.path = path};
        folder[parentWd].children[name] = wd;

        addChildren(wd,path);
        return path;
    };
    std::function<std::filesystem::path(WatchDescriptor wd,std::string name)> getPath = [&](WatchDescriptor parentWd,std::string name)->std::filesystem::path{
        assert(folder.find(parentWd) != folder.end());
        return folder[parentWd].path/name;
    };


    std::function<std::filesystem::path(WatchDescriptor wd,std::string name)> unregisterPath = [&](WatchDescriptor parentWd,std::string name)->std::filesystem::path{
        assert(folder.find(parentWd) != folder.end());
        auto& parent = folder[parentWd];
        auto path = parent.path / name;
        assert(parent.children.find(name) != parent.children.end());
        auto& wd = parent.children[name];
        assert(folder.find(wd) != folder.end());
        // recursivly
        
        std::function<void(WatchDescriptor wd)> remove;
        remove = [&](WatchDescriptor wd){
            std::vector<WatchDescriptor> wds;
            for (auto pair : folder[wd].children) {
                wds.push_back(pair.second);
            }
            inotify_rm_watch(fd, wd);
            folder.erase(wd);
            for(auto wd:wds)
                remove(wd);
        };
        remove(wd);        
        return path;
    };

    // add root
    int wd = inotify_add_watch(fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE);
    folder[wd] = {.path = path};
    addChildren(wd,path);



    mutex.lock();
    while(!token.stop_requested()){        
        mutex.unlock();
        length = read( fd, buffer, BUF_LEN );  
        mutex.lock();
      
        if ( length < 0 ) {
            perror( "read" );
        }  
        int i = 0;
        while ( i < length ) {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            if ( event->len ) {
                if ( event->mask & IN_CREATE ) {
                    std::filesystem::path path;
                    if(event->mask & IN_ISDIR)
                        path = registerPath(event->wd,event->name);
                    else 
                        path = getPath(event->wd, event->name);

                    commands.push_back(Command{
                        .state = Command::CREATE,
                        .time  = std::chrono::steady_clock::now(),
                        .path1 = path
                    });
                } else if ( event->mask & IN_DELETE ) {
                    std::filesystem::path path;
                    if(event->mask & IN_ISDIR)
                        path = unregisterPath(event->wd,event->name);
                    else 
                        path = getPath(event->wd, event->name);

                    commands.push_back(Command{
                        .state = Command::REMOVE,
                        .time = std::chrono::steady_clock::now(),
                        .path1 = path
                    });
                }else if( event->mask & IN_MOVED_FROM) {
                    std::filesystem::path path;
                    if(event->mask & IN_ISDIR)
                        path = unregisterPath(event->wd,event->name);
                    else 
                        path = getPath(event->wd, event->name);

                    commands.push_back(Command{
                        .state = Command::_MOVE_FROM,
                        .time = std::chrono::steady_clock::now(),
                        .cookie = event->cookie,
                        .path1 = path
                    });
                }else if( event->mask & IN_MOVED_TO) { // THIS MIGHT NOT EVEN TRIGGER AFTER IN_MOVED_FROM. use a 5 second timeout
                     std::filesystem::path path;
                    if(event->mask & IN_ISDIR)
                        path = registerPath(event->wd,event->name);
                    else 
                        path = getPath(event->wd, event->name);
                    
                    bool found = false;
                    for(auto& cmd:commands){
                        if(cmd.state == Command::_MOVE_FROM && cmd.cookie == event->cookie){
                            cmd.state = Command::MOVE;
                            cmd.path2 = path;
                            found = true;
                            break;
                        }
                    }
                    if(!found){
                        commands.push_back(Command{
                            .state = Command::CREATE,
                            .time = std::chrono::steady_clock::now(),
                            .cookie = event->cookie,
                            .path1  = path
                        });
                    }
                }else if ( event->mask & IN_MODIFY ) { // might called multiple times, so wait 100ms before actually calling callbacks, just to avoid duplicate calls.
                    assert((event->mask & IN_ISDIR) == false);
                    auto path = getPath(event->wd, event->name);
                    
                    commands.push_back(Command{
                        .state = Command::MODIFY,
                        .time  = std::chrono::steady_clock::now(),
                        .path1 = path
                    });
                }
            }
            i += EVENT_SIZE + event->len;
        }

        
    }
    mutex.unlock();

    for(auto& pair:folder){
        inotify_rm_watch(fd, pair.first);
    }
    close( fd );
#elif WIN32
    auto dirPath = path.wstring();
    assert(handle == INVALID_HANDLE_VALUE);
    handle = CreateFileW(
        dirPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );
    if(handle == INVALID_HANDLE_VALUE){
        std::cerr << "Error while opening directory: " << GetLastError() << std::endl;
        std::wcerr << L"path: "<<path.native().c_str() << std::endl;
        return;
    }

    DWORD length = 0;
    BYTE buffer[BUF_LEN];

    std::wstring lastPath;
    mutex.lock();
    while(!token.stop_requested()){
        mutex.unlock();
        if(ReadDirectoryChangesW(
            handle,
            buffer,
            BUF_LEN,
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &length,
            NULL,
            NULL
        )){
            auto* event = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(buffer);
            while (event != nullptr) {
                std::wstring fileName(event->FileName, event->FileNameLength / sizeof(WCHAR));
                
                std::filesystem::path filepath = dirPath; 
                filepath /= fileName;

                switch(event->Action){
                    case FILE_ACTION_ADDED:            
                        commands.push_back(Command{
                            .state = Command::CREATE,
                            .time  = std::chrono::steady_clock::now(),
                            .path1 = filepath
                        });
                    break;
                    case FILE_ACTION_REMOVED:          
                        commands.push_back(Command{
                            .state = Command::REMOVE,
                            .time  = std::chrono::steady_clock::now(),
                            .path1 = filepath
                        });
                    break;
                    case FILE_ACTION_MODIFIED:         
                        commands.push_back(Command{
                            .state = Command::MODIFY,
                            .time  = std::chrono::steady_clock::now(),
                            .path1 = filepath
                        });
                    break;
                    case FILE_ACTION_RENAMED_OLD_NAME: 
                        lastPath = filepath.wstring();
                    break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        commands.push_back(Command{
                            .state = Command::MOVE,
                            .time  = std::chrono::steady_clock::now(),
                            .path1 = lastPath,
                            .path2 = filepath.wstring()
                        });
                    ;break;
                }
                //std::wcout << filepath << std::endl;

                if(event->NextEntryOffset == 0){
                    event = nullptr;
                }else{
                    event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<BYTE*>(event) + event->NextEntryOffset
                    );
                }
            }
        }

        mutex.lock();
    }
    mutex.unlock();
    CloseHandle(handle);
#endif 
}
std::vector<FilesystemWatch::Command> FilesystemWatch::getCommand(){
    int end = 0;
    mutex.lock();
    while (commands.size()>end && commands[end].time < std::chrono::steady_clock::now()+std::chrono::milliseconds(20)) {
        auto& command = commands[end];
        if(command.state == Command::_MOVE_FROM)
            command.state = Command::REMOVE;
        end++;
    }
    auto rv = std::vector<Command>(commands.begin(),commands.begin()+end);
    commands.erase(commands.begin(),commands.begin()+end);
    mutex.unlock();

    return rv;
}