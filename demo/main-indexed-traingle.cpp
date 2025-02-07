#include <iostream>
#include "VK_Context.h"
#include "VK_Pipeline.h"

using namespace std;

const std::vector<float> vertices = {
    0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f,
    0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f,
    -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f,
    -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f
};

const std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 1
};

VK_Pipeline *pipeline = nullptr;
VK_Context *context = nullptr;

void onMouseButtonCallback(int button, int action, int mods)
{
    VkPipelineColorBlendAttachmentState blend{};
    if (button) {
        blend.blendEnable = VK_TRUE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        blend.blendEnable = VK_FALSE;
        //context->setColorBlendAttachmentState(blend);
    }
}

int main()
{
    VK_ContextConfig config;
    config.debug = true;
    config.name = "Indexed-Triangle";
    config.mouseCallback = &onMouseButtonCallback;

    context = createVkContext(config);
    context->createWindow(640, 480, true);

    VK_Context::VK_Config vkConfig;
    context->initVulkanDevice(vkConfig);

    auto shaderSet = context->createShaderSet();
    shaderSet->addShader("../../shader/vertex/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderSet->addShader("../../shader/vertex/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    shaderSet->appendVertexAttributeDescription(0, sizeof (float) * 3, VK_FORMAT_R32G32B32_SFLOAT, 0);
    shaderSet->appendVertexAttributeDescription(1, sizeof (float) * 4, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                sizeof(float) * 3);

    shaderSet->appendVertexInputBindingDescription(7 * sizeof(float), 0, VK_VERTEX_INPUT_RATE_VERTEX);

    if (!shaderSet->isValid()) {
        std::cerr << "invalid shaderSet" << std::endl;
        shaderSet->release();
        context->release();
        return -1;
    }

    context->initVulkanContext();
    auto pipeline = context->createPipeline(shaderSet);
    pipeline->create();

    auto buffer = context->createVertexBuffer(vertices, 12, indices);
    pipeline->addRenderBuffer(buffer);

    context->createCommandBuffers();

    context->run();
    context->release();

    return 0;
}
