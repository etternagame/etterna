#ifndef RENDERER_DX12_H
#define RENDERER_DX12_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <directx/d3dx12.h>
#include <D3D12MemAlloc.h>
#include <DirectXMath.h>
#include <dxcapi.h>

#include "BufferHelperDX12.h"
#include "RageUtil/Graphics/Display/Display.h"
#include "RageUtil/Graphics/Display/Renderer.h"

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
    void WaitForGPU();
    void OnDestroy();
    void PopulateCommandList(const ActualVideoModeParams *p);
    void RunIndirectCommandShader(const Display::CommandBatcher &batcher);

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
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
    UINT m_RtvDescriptorSize;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTargets[Display::Display::FrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

    struct PipelineHelpers
    {
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
    };

    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    uint64_t m_FenceValue = 0;
    UINT m_FrameIndex = 0;
    HANDLE m_FenceEvent;

    PipelineHelpers m_GraphicsHelpers;
    PipelineHelpers m_ComputeHelpers;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    static constexpr D3D12_RESOURCE_DESC GetTextureDescription();
    static constexpr size_t MaxDrawCommands = 20'000;
    static constexpr size_t MaxVertices = MaxDrawCommands * 5U;
    static constexpr size_t ComputeShaderThreadCount = 64;

    std::vector<Display::TextureCommand> m_TextureCommandQueue;
    intptr_t m_TextureIndex;

    Microsoft::WRL::ComPtr<IDxcBlob> m_IndirectCommandShader;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_IndirectCommandHeap;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_IndirectCommandSignature;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputCommandBuffer;
    // less of a mouthful than CBV_SRV_UAV >:3
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_MintyFreshHeap;
    UINT m_MintyFreshDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE m_MintyFreshHeapCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_MintyFreshHeapGpuHandle;

    enum DescriptorHeapOffsets
    {
        IndirectCommandSrv,
        MatrixStateSrv,
        RenderStateSrv,
        TextureSrv,
		IndirectCommandArgSrv,
		OutputCommandUav,
        DescriptorCount,
    };

    static constexpr UINT MintyFreshDescriptorCount = DescriptorHeapOffsets::DescriptorCount;

    std::unique_ptr<BufferHelperDX12<Display::DrawCommand>> m_IndirectCommandHelper;
    std::unique_ptr<BufferHelperDX12<Display::DrawCommandArgument>> m_IndirectCommandArgumentHelper;
    std::unique_ptr<BufferHelperDX12<Display::RenderState>> m_RenderStateHelper;
    std::unique_ptr<BufferHelperDX12<Display::MatrixState>> m_MatrixStateHelper;

    std::unique_ptr<BufferHelperDX12<RageSpriteVertex>> m_RageSpriteVertexHelper;

    void InitUploadBufferHelpers();
    void UploadBatchToBufferHelpers(const Display::CommandBatcher &batcher);
    void CopyHelperDataToDestBuffers();
    void CreateViewsForBufferHelpers();

    Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureStub;
	void CreateTextureStub();
};

#endif
