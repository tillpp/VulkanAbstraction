#pragma  once
#include "engine/vulkan/common.hpp" // IWYU pragma: keep

struct DefaultVertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return {.binding = 0, .stride = sizeof(DefaultVertex), .inputRate = vk::VertexInputRate::eVertex};
    }
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
    {
      return {{{.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(DefaultVertex, pos)},
               {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(DefaultVertex, color)},
               {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(DefaultVertex, texCoord)}}};
    }
};