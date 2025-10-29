#include "SynthwaveApplication.h"

#include "Rendering/CommandBuffer.h"
#include "Rendering/ComputePipeline.h"
#include "Rendering/GraphicsPipeline.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderBindingSet.h"
#include "Rendering/ShaderEntryPoint.h"
#include "Rendering/Texture.h"
#include "Runtime/EntryPoint.h"
#include "Runtime/Path.h"

#include <vulkan/vulkan.h>

#include "Rendering/Vulkan/CommandBuffer.h"
#include "Rendering/Vulkan/Texture.h"
#include "Runtime/Time.h"

NOVA_DEFINE_APPLICATION_CLASS(SynthwaveApplication);

using namespace Nova;

ApplicationConfiguration SynthwaveApplication::GetConfiguration() const
{
    ApplicationConfiguration config;
    config.applicationName = "Synthwave Compute | Nova Engine";
    config.windowFlags = WindowCreateFlagBits::Default;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.vsync = false;
    return config;
}


void SynthwaveApplication::OnInit()
{
    Ref<Rendering::Device>& device = GetDevice();
    const uint32_t width = GetWindowWidth();
    const uint32_t height = GetWindowHeight();

    // Create texture
    Rendering::TextureCreateInfo texCreateInfo = Rendering::TextureCreateInfo()
    .withFlags(Rendering::TextureUsageFlagBits::Storage | Rendering::TextureUsageFlagBits::Sampled)
    .withWidth(width)
    .withHeight(height)
    .withDepth(1)
    .withFormat(Format::R32G32B32A32_FLOAT)
    .withMips(1)
    .withSampleCount(1);
    m_Texture = device->CreateTexture(texCreateInfo);

    // Create sampler
    Rendering::SamplerCreateInfo samplerCreateInfo = Rendering::SamplerCreateInfo()
    .withFilter(Filter::Linear, Filter::Linear)
    .withAddressMode(SamplerAddressMode::Repeat);
    m_Sampler = device->CreateSampler(samplerCreateInfo);

    // Compile and load fullscreen shader
    Rendering::ShaderCreateInfo fullscreenShaderCreateInfo = Rendering::ShaderCreateInfo()
    .withTarget(Rendering::ShaderTarget::SPIRV)
    .withEntryPoints({{"vert", ShaderStageFlagBits::Vertex}, {"frag", ShaderStageFlagBits::Fragment}})
    .withModuleInfo({"Fullscreen", Path::GetAssetPath("Fullscreen.slang")})
    .withSlang(GetSlangSession());
    m_FullscreenShader = device->CreateShader(fullscreenShaderCreateInfo);

    // Create Graphics Pipeline
    const Rendering::GraphicsPipelineCreateInfo gpCreateInfo = Rendering::GraphicsPipelineCreateInfo()
    .setShader(m_FullscreenShader)
    .setRenderPass(GetRenderPass())
    .setViewportInfo({0, 0, width, height, 0.0f, 1.0f})
    .setScissorInfo({0, 0, width, height})
    .setMultisampleInfo({8});
    m_FullscreenPipeline = device->CreateGraphicsPipeline(gpCreateInfo);

    // Compile and load synthwave shader
    Rendering::ShaderCreateInfo synthwaveShaderCreateInfo = Rendering::ShaderCreateInfo()
    .withTarget(Rendering::ShaderTarget::SPIRV)
    .withEntryPoints({Rendering::ShaderEntryPoint("compute", ShaderStageFlagBits::Compute)})
    .withModuleInfo({"Synthwave", Path::GetAssetPath("Synthwave.slang")})
    .withSlang(GetSlangSession());
    m_SynthwaveShader = device->CreateShader(synthwaveShaderCreateInfo);

    // Create compute pipeline
    Rendering::ComputePipelineCreateInfo cpCreateInfo = Rendering::ComputePipelineCreateInfo()
        .withShader(m_SynthwaveShader);
    m_ComputePipeline = device->CreateComputePipeline(cpCreateInfo);

    // Creating binding sets
    m_FullscreenBindingSet = m_FullscreenShader->CreateBindingSet();
    m_SynthwaveBindingSet = m_SynthwaveShader->CreateBindingSet();
}

void SynthwaveApplication::OnDestroy()
{
    m_ComputePipeline->Destroy();
    m_FullscreenPipeline->Destroy();
    m_SynthwaveShader->Destroy();
    m_FullscreenShader->Destroy();
    m_Texture->Destroy();
    m_Sampler->Destroy();
    m_FullscreenBindingSet->Destroy();
    m_SynthwaveBindingSet->Destroy();
}

void SynthwaveApplication::OnPreRender(Rendering::CommandBuffer& cmdBuffer)
{
    cmdBuffer.BindComputePipeline(*m_ComputePipeline);
    cmdBuffer.BindShaderBindingSet(*m_SynthwaveShader, *m_SynthwaveBindingSet);

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = m_Texture.As<Vulkan::Texture>()->GetImage();
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_Texture->GetMips();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkCmdPipelineBarrier(((Vulkan::CommandBuffer&)cmdBuffer).GetHandle(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_SynthwaveBindingSet->BindTexture(0, m_Texture);

    constexpr uint32_t workGroupSizeX = 16;
    constexpr uint32_t workGroupSizeY = 16;

    const uint32_t numGroupsX = (m_Texture->GetWidth() + workGroupSizeX - 1) / workGroupSizeX;
    const uint32_t numGroupsY = (m_Texture->GetHeight() + workGroupSizeY - 1) / workGroupSizeY;

    const float constants[2] = { (float)Time::Get(), GetDeltaTime() };
    cmdBuffer.PushConstants(*m_SynthwaveShader, ShaderStageFlagBits::Compute, 0, sizeof(constants), constants);
    cmdBuffer.Dispatch(numGroupsX, numGroupsY, 1);

    VkImageMemoryBarrier barrier2 = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier2.image = m_Texture.As<Vulkan::Texture>()->GetImage();
    barrier2.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier2.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier2.subresourceRange.baseMipLevel = 0;
    barrier2.subresourceRange.levelCount = m_Texture->GetMips();
    barrier2.subresourceRange.baseArrayLayer = 0;
    barrier2.subresourceRange.layerCount = 1;
    barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkCmdPipelineBarrier(((Vulkan::CommandBuffer&)cmdBuffer).GetHandle(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
}

void SynthwaveApplication::OnRender(Rendering::CommandBuffer& cmdBuffer)
{
    const Rendering::RenderPass* renderPass = GetRenderPass();

    cmdBuffer.BindGraphicsPipeline(*m_FullscreenPipeline);
    cmdBuffer.BindShaderBindingSet(*m_FullscreenShader, *m_FullscreenBindingSet);
    m_FullscreenBindingSet->BindCombinedSamplerTexture(0, m_Sampler, m_Texture);

    cmdBuffer.SetViewport(renderPass->GetOffsetX(), renderPass->GetOffsetY(), renderPass->GetWidth(), renderPass->GetHeight(), 0.0f, 1.0f);
    cmdBuffer.SetScissor(renderPass->GetOffsetX(), renderPass->GetOffsetY(), renderPass->GetWidth(), renderPass->GetHeight());
    cmdBuffer.Draw(6, 1, 0, 0);
}

void SynthwaveApplication::OnGUI()
{
}
