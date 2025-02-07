#include <iostream>
#include <cstring>
#include <chrono>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include "VK_UniformBuffer.h"
#include "VK_Context.h"
#include "VK_Image.h"
#include "VK_Texture.h"
#include "VK_Pipeline.h"
#include "VK_DynamicState.h"

using namespace std;

VK_Context *context = nullptr;
VK_Pipeline *pipeline = nullptr;

uint32_t updateUniformBufferData(char *&data, uint32_t size)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>
                 (currentTime - startTime).count();
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model *= glm::rotate(glm::mat4(1.0f), time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    auto view = glm::lookAt(glm::vec3(0.0f, 4.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,
                            0.0f, 1.0f));
    auto proj = glm::perspective(glm::radians(45.0f),
                                 context->getSwapChainExtent().width / (float)context->getSwapChainExtent().height, 0.1f, 10.0f);
    proj[1][1] *= -1;

    model = proj * view * model;
    memcpy(data, &model[0][0], size);

    time = sin(time);
    memcpy(data + sizeof(float) * 16, (void *)&time, sizeof(float));
    return 17 * sizeof(float);
}

void onFrameSizeChanged(int width, int height)
{
    pipeline->getDynamicState()->applyDynamicViewport({0, 0, (float)width, (float)height, 0, 1});
}

auto queryCallback = [](const std::vector<uint64_t> &data)
{
    assert(data.size() == 8);
    std::vector<std::string> items = {
        "Vertex count",
        "Primitives count",
        "Vert shader invocations",
        "Clipping invocations",
        "Clipping primtives",
        "Frag shader invocations",
        "Tessellation control shader patches",
        "Tessellation evaluation shader invocations"
    };

    for (int i = 0; i < 8; i++)
        std::cout << items[i] << ":" << data[i] << std::endl;
    std::cout << std::endl;
};

int main()
{
    VK_ContextConfig config;
    config.debug = true;
    config.name = "Model Mesh Tess";

    context = createVkContext(config);
    context->createWindow(480, 480, true);
    context->setOnFrameSizeChanged(onFrameSizeChanged);

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.tessellationShader = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.pipelineStatisticsQuery = VK_TRUE;
    context->setLogicalDeviceFeatures(deviceFeatures);

    VK_Context::VK_Config vkConfig;
    context->initVulkanDevice(vkConfig);

    auto shaderSet = context->createShaderSet();
    shaderSet->addShader("../../shader/geom-mesh-tess/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderSet->addShader("../../shader/geom-mesh-tess/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    shaderSet->addShader("../../shader/geom-mesh-tess/tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    shaderSet->addShader("../../shader/geom-mesh-tess/tese.spv",
                         VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

    shaderSet->appendVertexAttributeDescription(0, sizeof (float) * 3, VK_FORMAT_R32G32B32_SFLOAT, 0);
    shaderSet->appendVertexAttributeDescription(1, sizeof (float) * 2, VK_FORMAT_R32G32_SFLOAT,
            sizeof(float) * 3);
    shaderSet->appendVertexAttributeDescription(2, sizeof (float) * 3, VK_FORMAT_R32G32B32_SFLOAT,
            sizeof(float) * 5);

    shaderSet->appendVertexInputBindingDescription(8 * sizeof(float), 0, VK_VERTEX_INPUT_RATE_VERTEX);

    VkDescriptorSetLayoutBinding uniformBinding = VK_ShaderSet::createDescriptorSetLayoutBinding(0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    shaderSet->addDescriptorSetLayoutBinding(uniformBinding);

    if (!shaderSet->isValid()) {
        std::cerr << "invalid shaderSet" << std::endl;
        shaderSet->release();
        context->release();
        return -1;
    }

    auto ubo = shaderSet->addUniformBuffer(0, sizeof(float) * 17);
    ubo->setWriteDataCallback(updateUniformBufferData);

    context->initVulkanContext();

    pipeline = context->createPipeline(shaderSet);

    auto tessellationCreateInfo = VK_Pipeline::createPipelineTessellationStateCreateInfo(32);
    tessellationCreateInfo.patchControlPoints = 3;

    auto inputAssmly = pipeline->getInputAssemblyStateCreateInfo();
    inputAssmly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

    pipeline->setInputAssemblyStateCreateInfo(inputAssmly);
    pipeline->setTessellationStateCreateInfo(tessellationCreateInfo);

    auto rasterCreateInfo = pipeline->getRasterizationStateCreateInfo();
    rasterCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
    pipeline->setRasterizationStateCreateInfo(rasterCreateInfo);

    pipeline->create();
    auto buffer = context->createVertexBuffer("../../model/pug.obj", true);
    pipeline->addRenderBuffer(buffer);

    auto query = context->createQueryPool(8, VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                                          VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                                          VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                                          VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                                          VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                                          VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                                          VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
                                          VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT, queryCallback);


    context->createCommandBuffers();

    context->run();
    context->release();

    return 0;
}



