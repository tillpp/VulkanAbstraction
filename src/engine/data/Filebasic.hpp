#pragma once
#include <string>
#include <filesystem>

bool readFile(const std::filesystem::path& filename,std::string& data);
std::string readFileOrThrow(const std::filesystem::path& filename);
bool writeFile(const std::filesystem::path& filename,std::string data);
void writeFileOrThrow(const std::filesystem::path& filename,std::string data);
