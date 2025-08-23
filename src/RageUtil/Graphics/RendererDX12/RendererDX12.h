#ifndef RENDERER_DX12_H
#define RENDERER_DX12_H

#include "RageUtil/Graphics/Display/Display.h"
#include "RageUtil/Graphics/Display/Renderer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <D3D12MemAlloc.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <directx/d3dx12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <windows.h>

class RendererDX12 : public Display::Renderer
{
  public:
    RendererDX12();
    ~RendererDX12() override;
    [[nodiscard]] std::string GetApiDescription() const override;
    void StartLoadingPipeline() override;
    void FinishLoadingPipeline(const VideoModeParams &p) override;
    void LoadAssets(const VideoModeParams &p) override;
    void OnRender(const ActualVideoModeParams *p, const Display::CommandBatcher &batcher) override;
    bool IsD3DInternal() override
    {
        return true;
    }
    intptr_t PushTextureCommand(const Display::TextureCommand &command) override;

  private:
    void SignalFence(bool waitForEvent);
    void OnDestroy();
    void PopulateCommandList(const ActualVideoModeParams *p);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateResource(
        const D3D12_RESOURCE_DESC &resourceDesc, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COPY_DEST);

    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::string &path, RageShaderType shaderType);
    Microsoft::WRL::ComPtr<IDxcCompiler3> m_ShaderCompiler;
    Microsoft::WRL::ComPtr<IDxcUtils> m_ShaderCompilerUtils;

    Microsoft::WRL::ComPtr<IDXGIFactory7> m_DXGIFactory;
    UINT m_DXGIFactoryFlags;

    Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_Allocator;
    Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
    UINT m_RtvDescriptorSize;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTargets[Display::Display::FrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

    UINT m_FrameIndex;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    uint64_t m_FenceValue;
    HANDLE m_FenceEvent;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    static constexpr D3D12_RESOURCE_DESC GetTextureDescription();
    static constexpr size_t MaxDrawCommands = 100'000;

    std::vector<Display::TextureCommand> m_TextureCommandQueue;
    intptr_t m_TextureIndex;

#pragma pack(push, 4)
    struct IndirectCommand
    {
        Display::DrawCommand draw;
        Display::DrawCommandArgument args;
    };
#pragma pack(pop)
    static_assert(sizeof(IndirectCommand) == 24,
                  "IndirectCommand size should match the HLSL compute shader definition");

	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndirectCommandHeap;
};

#endif
