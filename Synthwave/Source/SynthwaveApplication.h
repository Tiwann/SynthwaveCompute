#pragma once
#include "Runtime/Application.h"

namespace Nova::Rendering
{
    class ShaderBindingSet;
}

using Nova::Ref;
using Nova::Rendering::Texture;
using Nova::Rendering::Shader;
using Nova::Rendering::Sampler;
using Nova::Rendering::ComputePipeline;
using Nova::Rendering::GraphicsPipeline;
using Nova::Rendering::ShaderBindingSet;

class SynthwaveApplication final : public Nova::Application
{
public:
    Nova::ApplicationConfiguration GetConfiguration() const override;
    void OnInit() override;
    void OnDestroy() override;
    void OnPreRender(Nova::Rendering::CommandBuffer& cmdBuffer) override;
    void OnRender(Nova::Rendering::CommandBuffer& cmdBuffer) override;
    void OnGUI() override;

private:
    Ref<Texture> m_Texture = nullptr;
    Ref<Sampler> m_Sampler = nullptr;
    Ref<Shader> m_SynthwaveShader = nullptr;
    Ref<Shader> m_FullscreenShader = nullptr;
    Ref<GraphicsPipeline> m_FullscreenPipeline = nullptr;
    Ref<ComputePipeline> m_ComputePipeline = nullptr;
    Ref<ShaderBindingSet> m_FullscreenBindingSet = nullptr;
    Ref<ShaderBindingSet> m_SynthwaveBindingSet = nullptr;
};
