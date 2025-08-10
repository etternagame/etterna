#include "RendererDX12.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Graphics/RageSurface.h"
#include "archutils/Win32/GraphicsWindow.h"
#include <chrono>
#include <exception>
#include <fstream>
#include <source_location>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

inline static std::string HrToString(HRESULT hr)
{
    char *errorMsg = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                   hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&errorMsg), 0, nullptr);

    std::string message = errorMsg ? errorMsg : "Unknown error";
    if (errorMsg)
        LocalFree(errorMsg);
    return message;
}

inline static void ThrowIfFailed(HRESULT hr, const std::source_location location = std::source_location::current())
{
    if (SUCCEEDED(hr))
    {
        return;
    }

    std::string error = HrToString(hr);
    const std::string message = std::format("Failed: HRESULT {} ({}) at {}:{} in function {}", hr, error,
                                            location.file_name(), location.line(), location.function_name());
    Locator::getLogger()->error(message);
    throw std::exception(message.c_str());
}

RendererDX12::RendererDX12() : m_TextureIndex(0) {}

RendererDX12::~RendererDX12()
{
    OnDestroy();
}

std::string RendererDX12::GetApiDescription() const
{
    return "DirectX12";
}

void RendererDX12::StartLoadingPipeline()
{
    GraphicsWindow::Initialize(true);
#if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
        m_DXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory)));

    HRESULT result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device));
    if (FAILED(result))
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(m_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)));
    }

    D3D12_COMMAND_QUEUE_DESC queueDescription = {};
    queueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_Device->CreateCommandQueue(&queueDescription, IID_PPV_ARGS(&m_CommandQueue)));
}

void RendererDX12::FinishLoadingPipeline(const VideoModeParams &p)
{
    DXGI_SWAP_CHAIN_DESC swapChainDescription = {};
    swapChainDescription.BufferCount = Display::Display::FrameCount;
    swapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDescription.OutputWindow = GraphicsWindow::GetHwnd();
    swapChainDescription.SampleDesc.Count = 1;
    swapChainDescription.Windowed = p.windowed;
    swapChainDescription.BufferDesc.Width = p.width;
    swapChainDescription.BufferDesc.Height = p.height;

    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(m_DXGIFactory->CreateSwapChain(m_CommandQueue.Get(), &swapChainDescription, &swapChain));
    ThrowIfFailed(swapChain.As(&m_SwapChain));

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescription = {};
        rtvHeapDescription.NumDescriptors = Display::Display::FrameCount;
        rtvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_Device->CreateDescriptorHeap(&rtvHeapDescription, IID_PPV_ARGS(&m_RtvHeap)));

        m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (UINT n = 0; n < Display::Display::FrameCount; n++)
        {
            ThrowIfFailed(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));
            m_Device->CreateRenderTargetView(m_RenderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_RtvDescriptorSize);
        }
    }

    ThrowIfFailed(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
}

static std::string ReadFileContents(const std::string &path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
    {
        Locator::getLogger()->error("ReadFileContents: file not found - {}", path);
        throw std::exception(("shader file not found: " + path).c_str());
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

ComPtr<ID3DBlob> CompileShader(const std::string &contents, const std::string &entrypoint,
                               const std::string &targetProfile)
{
    ComPtr<ID3DBlob> errorBlob;
    ComPtr<ID3DBlob> shaderBlob;

    UINT shaderCompileFlags = 0; // D3DCOMPILE_ENABLE_STRICTNESS maybe?
#if defined(DEBUG) || defined(_DEBUG)
    shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    auto hr = D3DCompile(contents.c_str(), contents.size(), "", nullptr, nullptr, entrypoint.c_str(),
                         targetProfile.c_str(), shaderCompileFlags, 0, &shaderBlob, &errorBlob);
    if (FAILED(hr) && errorBlob)
    {
        Locator::getLogger()->error("Failed to compile shader ({}) - {}", targetProfile,
                                    (char *)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    return shaderBlob;
}

void RendererDX12::LoadAssets(const VideoModeParams &p)
{
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(
            D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_RootSignature)));
    }

    {
        // TODO: switching or SPIR-V or something later
        std::string shaderPath = FILEMAN->ResolvePath("Data/Shaders/HLSL/shaders.hlsl").substr(1);

        std::string shaderContents = ReadFileContents(shaderPath);

        ComPtr<ID3DBlob> vertexShader = CompileShader(shaderContents, "VSMain", "vs_5_0");
        ComPtr<ID3DBlob> pixelShader = CompileShader(shaderContents, "PSMain", "ps_5_0");

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
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
        ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)));
    }

    ThrowIfFailed(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(),
                                              m_PipelineState.Get(), IID_PPV_ARGS(&m_CommandList)));

    ThrowIfFailed(m_CommandList->Close());

    {
        Vertex triangleVertices[] = {{{0.0f, 0.25f * p.fDisplayAspectRatio, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                                     {{0.25f, -0.25f * p.fDisplayAspectRatio, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                                     {{-0.25f, -0.25f * p.fDisplayAspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

        const UINT vertexBufferSize = sizeof(triangleVertices);

        auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(m_Device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &buffer,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&m_VertexBuffer)));

        UINT8 *pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_VertexBuffer->Unmap(0, nullptr);

        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.StrideInBytes = sizeof(Vertex);
        m_VertexBufferView.SizeInBytes = vertexBufferSize;
    }

    {
        auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferProps = CD3DX12_RESOURCE_DESC::Buffer(
            Display::Display::MaxTextureSize * Display::Display::MaxTextureSize * Display::Display::TexturePixelSize);
        ThrowIfFailed(m_Device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferProps,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&m_TextureUploadHeap)));
    }

    {
        ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
        m_FenceValue = 1;

        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_FenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        SignalFence(true);
    }
}

void RendererDX12::PopulateCommandList(const ActualVideoModeParams *p)
{
    ThrowIfFailed(m_CommandAllocator->Reset());
    ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), m_PipelineState.Get()));

    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

    m_Viewport = D3D12_VIEWPORT(0.0f, 0.0f, static_cast<float>(p->width), static_cast<float>(p->height));
    m_ScissorRect = D3D12_RECT(0, 0, static_cast<LONG>(p->width), static_cast<LONG>(p->height));

    m_CommandList->RSSetViewports(1, &m_Viewport);
    m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_RenderTargets[m_FrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_CommandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex,
                                            m_RtvDescriptorSize);

    m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const auto clock = std::chrono::steady_clock::now();
    const auto time = std::chrono::time_point_cast<std::chrono::milliseconds>(clock);
    const auto factor = std::sin(time.time_since_epoch().count() / 250.0f);

    const float clearColor[] = {0.0f, 0.4f, 0.4f + 0.2f * factor, 1.0f};
    m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    m_CommandList->DrawInstanced(3, 1, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_CommandList->ResourceBarrier(1, &barrier);
    ThrowIfFailed(m_CommandList->Close());
}

intptr_t
RendererDX12::PushTextureCommand(const Display::TextureCommand& command)
{
	m_TextureCommandQueue.push_back(command);
	return command.index() == Display::TextureCommandType::Creation
			 ? m_TextureIndex++
			 : 0;
}

void
RendererDX12::SignalFence(bool waitForEvent)
{
    const uint64_t fence = m_FenceValue;
    ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fence));
    m_FenceValue++;

    if (waitForEvent && m_Fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void RendererDX12::OnUpdate()
{
}

void RendererDX12::OnRender(const ActualVideoModeParams *p)
{
    PopulateCommandList(p);

    ID3D12CommandList *CommandLists[] = {m_CommandList.Get()};
    m_CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

    ThrowIfFailed(m_SwapChain->Present(1, 0));

    // this is... slow?
    // TODO: synchronize in a sane way
    SignalFence(true);
}

void RendererDX12::OnDestroy()
{
    SignalFence(true);
    CloseHandle(m_FenceEvent);
}

constexpr D3D12_RESOURCE_DESC RendererDX12::GetTextureDescription()
{
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = Display::Display::MaxTextureSize;
    textureDesc.Height = Display::Display::MaxTextureSize;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    return textureDesc;
}
