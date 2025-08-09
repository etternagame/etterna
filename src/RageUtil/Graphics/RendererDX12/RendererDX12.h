#ifndef RENDERER_DX12_H
#define RENDERER_DX12_H

#include "RageUtil/Graphics/Display/Renderer.h"
#include "RageUtil/Graphics/Display/Display.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <directx/d3dx12.h>

class RendererDX12 : public Display::Renderer
{
  public:
   	~RendererDX12() override;
	[[nodiscard]] std::string GetApiDescription() const override;
	void StartLoadingPipeline() override;
	void FinishLoadingPipeline(const VideoModeParams& p) override;
	void LoadAssets(const VideoModeParams& p) override;
	void WaitForPreviousFrame() override;
	void OnUpdate() override;
	void OnRender(const ActualVideoModeParams* p) override;

  private:
	void OnDestroy();
	void PopulateCommandList(const ActualVideoModeParams* p);

	Microsoft::WRL::ComPtr<IDXGIFactory7> m_DXGIFactory;
	UINT m_DXGIFactoryFlags;
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

	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	std::atomic_uint64_t m_ActorCount;
};

#endif
