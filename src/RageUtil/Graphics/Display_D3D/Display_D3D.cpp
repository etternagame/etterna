#include "Display_D3D.h"
#include "Core/Services/Locator.hpp"
#include "archutils/Win32/GraphicsWindow.h"
#include <source_location>
#include <exception>
#include "RageUtil/File/RageFileManager.h"
#include <fstream>
#include <chrono>
#include "RageUtil/Graphics/RageSurface.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

inline static std::string
HrToString(HRESULT hr)
{
	char* errorMsg = nullptr;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
					 FORMAT_MESSAGE_IGNORE_INSERTS,
				   nullptr,
				   hr,
				   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				   reinterpret_cast<LPSTR>(&errorMsg),
				   0,
				   nullptr);

	std::string message = errorMsg ? errorMsg : "Unknown error";
	if (errorMsg)
		LocalFree(errorMsg);
	return message;
}

inline static void
ThrowIfFailed(
  HRESULT hr,
  const std::source_location location = std::source_location::current())
{
	if (SUCCEEDED(hr)) {
		return;
	}

	std::string error = HrToString(hr);
	const std::string message =
	  std::format("Failed: HRESULT {} ({}) at {}:{} in function {}",
				  hr,
				  error,
				  location.file_name(),
				  location.line(),
				  location.function_name());
	Locator::getLogger()->error(message);
	throw std::exception(message.c_str());
}

Display_D3D::Display_D3D() = default;

Display_D3D::~Display_D3D()
{
	OnDestroy();
}

std::string
Display_D3D::Init(VideoModeParams&& p, bool bAllowUnacceleratedRenderer)
{
	Locator::getLogger()->info("Display_D3D::Init()");
	Locator::getLogger()->info(
	  "Current renderer: Direct3D (unstable DirectX 12 version)");

	GraphicsWindow::Initialize(true);
	StartLoadingPipeline();

	bool ignored = false;
	return SetVideoMode(std::move(p), ignored);
}

void
Display_D3D::GetDisplaySpecs(DisplaySpecs& out) const
{
}

void
Display_D3D::ResolutionChanged()
{
	RageDisplay::ResolutionChanged();
}

const RageDisplay::RagePixelFormatDesc*
Display_D3D::GetPixelFormatDesc(RagePixelFormat pf) const
{
	assert(pf == RagePixelFormat_RGBA8);
	static auto desc =
	  RagePixelFormatDesc{ 32,
						   { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF } };
	return &desc;
}

bool
Display_D3D::BeginFrame()
{
	return m_IsInitDone;
}

void
Display_D3D::EndFrame()
{
	OnUpdate();
	OnRender();
}

const ActualVideoModeParams*
Display_D3D::GetActualVideoModeParams() const
{
#ifdef _WIN32
	return GraphicsWindow::GetParams();
#else
#error Display_D3D is meant for Windows... Or something
#endif
}

void
Display_D3D::SetBlendMode(BlendMode mode)
{
}

bool
Display_D3D::SupportsTextureFormat(RagePixelFormat pixfmt, bool realtime)
{
	return pixfmt == RagePixelFormat_RGBA8;
}

bool
Display_D3D::SupportsThreadedRendering()
{
	return false;
}

bool
Display_D3D::SupportsPerVertexMatrixScale()
{
	return false;
}

intptr_t
Display_D3D::CreateTexture(RagePixelFormat pixfmt,
						   RageSurface* img,
						   bool bGenerateMipMaps)
{
	assert(pixfmt == RagePixelFormat_RGBA8);

	D3D12_RESOURCE_DESC textureDescription = {};
	textureDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDescription.Width = img->w;
	textureDescription.Height = img->h;
	textureDescription.DepthOrArraySize = 1;
	textureDescription.MipLevels = 1;
	textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format (RGBA8)
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;
	textureDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* texture = nullptr;
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(
	  m_Device->CreateCommittedResource(&heapProperties,
										D3D12_HEAP_FLAG_NONE,
										&textureDescription,
										D3D12_RESOURCE_STATE_COPY_DEST,

										nullptr,
										IID_PPV_ARGS(&texture)));

	return reinterpret_cast<intptr_t>(texture);
}

void
Display_D3D::UpdateTexture(intptr_t uTexHandle,
						   RageSurface* img,
						   int xoffset,
						   int yoffset,
						   int width,
						   int height)
{
	ID3D12Resource* texture = reinterpret_cast<ID3D12Resource*>(uTexHandle);
	// todo...
}

void
Display_D3D::DeleteTexture(intptr_t iTexHandle)
{
	ID3D12Resource* texture = reinterpret_cast<ID3D12Resource*>(iTexHandle);
	texture->Release();
}

void
Display_D3D::ClearAllTextures()
{
}

int
Display_D3D::GetNumTextureUnits()
{
	return TextureUnit::NUM_TextureUnit;
}

void
Display_D3D::SetTexture(TextureUnit tu, intptr_t iTexture)
{
}

void
Display_D3D::SetTextureMode(TextureUnit tu, TextureMode tm)
{
}

void
Display_D3D::SetTextureWrapping(TextureUnit tu, bool b)
{
}

int
Display_D3D::GetMaxTextureSize() const
{
	constexpr static int maxFor11_0 =
	  4096; // technically 16384 for 11_0 but nope
	return maxFor11_0;
}

void
Display_D3D::SetTextureFiltering(TextureUnit tu, bool b)
{
}

bool
Display_D3D::IsZWriteEnabled() const
{
	return false;
}

bool
Display_D3D::IsZTestEnabled() const
{
	return false;
}

void
Display_D3D::SetZWrite(bool b)
{
}

void
Display_D3D::SetZBias(float f)
{
}

void
Display_D3D::SetZTestMode(ZTestMode mode)
{
}

void
Display_D3D::ClearZBuffer()
{
}

void
Display_D3D::SetCullMode(CullMode mode)
{
}

void
Display_D3D::SetAlphaTest(bool b)
{
}

void
Display_D3D::SetMaterial(const RageColor& emissive,
						 const RageColor& ambient,
						 const RageColor& diffuse,
						 const RageColor& specular,
						 float shininess)
{
}

void
Display_D3D::SetLighting(bool b)
{
}

void
Display_D3D::SetLightOff(int index)
{
}

void
Display_D3D::SetLightDirectional(int index,
								 const RageColor& ambient,
								 const RageColor& diffuse,
								 const RageColor& specular,
								 const RageVector3& dir)
{
}

intptr_t
Display_D3D::CreateRenderTarget(const RenderTargetParam& param,
								int& iTextureWidthOut,
								int& iTextureHeightOut)
{
	return intptr_t();
}

intptr_t
Display_D3D::GetRenderTarget()
{
	return intptr_t();
}

void
Display_D3D::SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture)
{
}

void
Display_D3D::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
}

void
Display_D3D::SetCelShaded(int stage)
{
}

RageCompiledGeometry*
Display_D3D::CreateCompiledGeometry()
{
	return nullptr;
}

void
Display_D3D::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
}

void
Display_D3D::DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawQuadStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawFanInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawTrianglesInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
											int iNumVerts)
{
}

void
Display_D3D::DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
										  int iMeshIndex)
{
}

std::string
Display_D3D::TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
{
	GraphicsWindow::CreateGraphicsWindow(p);
	FinishLoadingPipeline(p);
	LoadAssets(p);

	ResolutionChanged();
	//OnRender() with a black clearing to not whiteblast people?

	m_IsInitDone = true;
	return std::string();
}

RageSurface*
Display_D3D::CreateScreenshot()
{
	return nullptr;
}

void
Display_D3D::StartLoadingPipeline()
{
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
		m_DXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory)));

	HRESULT result = D3D12CreateDevice(
	  nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device));
	if (FAILED(result)) {
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(
		  m_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
		  warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)));
	}

	D3D12_COMMAND_QUEUE_DESC queueDescription = {};
	queueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_Device->CreateCommandQueue(&queueDescription,
											   IID_PPV_ARGS(&m_CommandQueue)));
}

void
Display_D3D::FinishLoadingPipeline(const VideoModeParams& p)
{
	DXGI_SWAP_CHAIN_DESC swapChainDescription = {};
	swapChainDescription.BufferCount = FrameCount;
	swapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDescription.OutputWindow = GraphicsWindow::GetHwnd();
	swapChainDescription.SampleDesc.Count = 1;
	swapChainDescription.Windowed = p.windowed;
	swapChainDescription.BufferDesc.Width = p.width;
	swapChainDescription.BufferDesc.Height = p.height;

	ComPtr<IDXGISwapChain> swapChain;
	ThrowIfFailed(m_DXGIFactory->CreateSwapChain(
	  m_CommandQueue.Get(), &swapChainDescription, &swapChain));
	ThrowIfFailed(swapChain.As(&m_SwapChain));

	m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescription = {};
		rtvHeapDescription.NumDescriptors = FrameCount;
		rtvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_Device->CreateDescriptorHeap(&rtvHeapDescription,
													 IID_PPV_ARGS(&m_RtvHeap)));

		m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(
		  D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		  m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT n = 0; n < FrameCount; n++) {
			ThrowIfFailed(
			  m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));
			m_Device->CreateRenderTargetView(
			  m_RenderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_RtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_Device->CreateCommandAllocator(
	  D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
}

static std::string
ReadFileContents(const std::string& path)
{
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file) {
		Locator::getLogger()->error("ReadFileContents: file not found - {}",
									path);
		throw std::exception(("shader file not found: " + path).c_str());
	}

	std::ostringstream stream;
	stream << file.rdbuf();
	return stream.str();
}

ComPtr<ID3DBlob>
CompileShader(const std::string& contents,
			  const std::string& entrypoint,
			  const std::string& targetProfile)
{
	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> shaderBlob;

	UINT shaderCompileFlags = 0; // D3DCOMPILE_ENABLE_STRICTNESS maybe?
#if defined(DEBUG) || defined(_DEBUG)
	shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	auto hr = D3DCompile(contents.c_str(),
						 contents.size(),
						 "",
						 nullptr,
						 nullptr,
						 entrypoint.c_str(),
						 targetProfile.c_str(),
						 shaderCompileFlags,
						 0,
						 &shaderBlob,
						 &errorBlob);
	if (FAILED(hr) && errorBlob) {
		Locator::getLogger()->error("Failed to compile shader ({}) - {}",
									targetProfile,
									(char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return shaderBlob;
}

void
Display_D3D::LoadAssets(const VideoModeParams& p)
{
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(
		  0,
		  nullptr,
		  0,
		  nullptr,
		  D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc,
												  D3D_ROOT_SIGNATURE_VERSION_1,
												  &signature,
												  &error));
		ThrowIfFailed(
		  m_Device->CreateRootSignature(0,
										signature->GetBufferPointer(),
										signature->GetBufferSize(),
										IID_PPV_ARGS(&m_RootSignature)));
	}

	{
		// TODO: switching or SPIR-V or something later
		std::string shaderPath =
		  FILEMAN->ResolvePath("Data/Shaders/HLSL/shaders.hlsl").substr(1);

		std::string shaderContents = ReadFileContents(shaderPath);

		ComPtr<ID3DBlob> vertexShader =
		  CompileShader(shaderContents, "VSMain", "vs_5_0");
		ComPtr<ID3DBlob> pixelShader =
		  CompileShader(shaderContents, "PSMain", "ps_5_0");

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION",
			  0,
			  DXGI_FORMAT_R32G32B32_FLOAT,
			  0,
			  0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			  0 },
			{ "COLOR",
			  0,
			  DXGI_FORMAT_R32G32B32A32_FLOAT,
			  0,
			  12,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			  0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs,
								_countof(inputElementDescs) };
		psoDesc.pRootSignature = m_RootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_Device->CreateGraphicsPipelineState(
		  &psoDesc, IID_PPV_ARGS(&m_PipelineState)));
	}

	ThrowIfFailed(m_Device->CreateCommandList(0,
											  D3D12_COMMAND_LIST_TYPE_DIRECT,
											  m_CommandAllocator.Get(),
											  m_PipelineState.Get(),
											  IID_PPV_ARGS(&m_CommandList)));

	ThrowIfFailed(m_CommandList->Close());

	{
		Vertex triangleVertices[] = {
			{ { 0.0f, 0.25f * p.fDisplayAspectRatio, 0.0f },
			  { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * p.fDisplayAspectRatio, 0.0f },
			  { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f * p.fDisplayAspectRatio, 0.0f },
			  { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(m_Device->CreateCommittedResource(
		  &properties,
		  D3D12_HEAP_FLAG_NONE,
		  &buffer,
		  D3D12_RESOURCE_STATE_GENERIC_READ,
		  nullptr,
		  IID_PPV_ARGS(&m_VertexBuffer)));

		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(
		  0, 0);
		ThrowIfFailed(m_VertexBuffer->Map(
		  0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_VertexBuffer->Unmap(0, nullptr);

		m_VertexBufferView.BufferLocation =
		  m_VertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.StrideInBytes = sizeof(Vertex);
		m_VertexBufferView.SizeInBytes = vertexBufferSize;
	}

	{
		ThrowIfFailed(m_Device->CreateFence(
		  0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
		m_FenceValue = 1;

		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		WaitForPreviousFrame();
	}
}

void
Display_D3D::PopulateCommandList()
{
	ThrowIfFailed(m_CommandAllocator->Reset());
	ThrowIfFailed(
	  m_CommandList->Reset(m_CommandAllocator.Get(), m_PipelineState.Get()));

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	auto p = GetActualVideoModeParams();
	m_Viewport = D3D12_VIEWPORT(
	  0.0f, 0.0f, static_cast<float>(p->width), static_cast<float>(p->height));
	m_ScissorRect = D3D12_RECT(
	  0, 0, static_cast<LONG>(p->width), static_cast<LONG>(p->height));

	m_CommandList->RSSetViewports(1, &m_Viewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	auto barrier =
	  CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
										   D3D12_RESOURCE_STATE_PRESENT,
										   D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CommandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
	  m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
	  m_FrameIndex,
	  m_RtvDescriptorSize);

	m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const auto clock = std::chrono::steady_clock::now();
	const auto time = std::chrono::time_point_cast<std::chrono::milliseconds>(clock);
	const auto factor = std::sin(time.time_since_epoch().count() / 300.0f);
	
	const float clearColor[] = { 0.0f, 0.4f, 0.4f + 0.2f * factor, 1.0f };
	m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_CommandList->IASetPrimitiveTopology(
	  D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	m_CommandList->DrawInstanced(3, 1, 0, 0);

	barrier =
	  CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
										   D3D12_RESOURCE_STATE_RENDER_TARGET,
										   D3D12_RESOURCE_STATE_PRESENT);
	m_CommandList->ResourceBarrier(1, &barrier);
	ThrowIfFailed(m_CommandList->Close());
}

void
Display_D3D::WaitForPreviousFrame()
{
	const uint64_t fence = m_FenceValue;
	ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fence));
	m_FenceValue++;

	if (m_Fence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}

	m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void
Display_D3D::OnUpdate()
{
}

void
Display_D3D::OnRender()
{
	PopulateCommandList();

	ID3D12CommandList* CommandLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	ThrowIfFailed(m_SwapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void
Display_D3D::OnDestroy()
{
	WaitForPreviousFrame();
	CloseHandle(m_FenceEvent);
}
