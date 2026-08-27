#pragma once
#include "common.hpp" // IWYU pragma: keep
#include <set>

struct InstanceSettings{
    std::set<std::string> extensions;
    std::set<std::string> layers;
    vk::InstanceCreateFlags flags{};
};
//TODO: vulkan give objects name
class Instance
{
    vk::raii::Context  context;
    vk::raii::Instance instance = nullptr;
public:
    operator vk::raii::Instance&();

    void create(InstanceSettings extensions);
    bool isCreated()const;
};