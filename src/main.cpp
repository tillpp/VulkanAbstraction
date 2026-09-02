
#include "App.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ostream>

#ifdef _WIN32
    #include <windows.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

void init(){
    setlocale(LC_ALL, "en_US.utf8");    
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    time_t t;
    time(&t);
    srand(t);
}
int main(int argc, char *argv[]){
    init();

    auto projectBaseDir = std::filesystem::canonical(argv[0]).parent_path().parent_path().parent_path();
    try{
        while(true){
            App app(projectBaseDir);
            if(!app.run())
                return 0;
            // reload for different mods.
        }
    } catch (const vk::SystemError& err){
        std::cerr << "Vulkan error: " << err.what() << std::endl;
        return 1;
    }catch (const std::exception& err){
        std::cerr << "Error: " << err.what() << std::endl;
        return 1;
    }
    return 0;
}
