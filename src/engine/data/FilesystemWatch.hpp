#pragma  once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

typedef int WatchDescriptor;
typedef uint32_t Cookie;    


//TODO: use select,poll,epoll,pipe
class FilesystemWatch{
    std::jthread thread;
#ifdef __linux__
    int fd = -1;
#elif WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
#endif
    std::mutex mutex;
public:
    class Command{
    public:
        enum CommandState{
            CREATE,
            REMOVE,
            MOVE,
            _MOVE_FROM,
            MODIFY,
        };
        CommandState state;
        std::chrono::steady_clock::time_point time;
        Cookie cookie;

        std::filesystem::path path1,path2;
    };
    FilesystemWatch(std::filesystem::path path);
    void run(std::stop_token token,std::filesystem::path path);
    std::vector<Command> getCommand();
private: 
    std::vector<Command> commands;
};
