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
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

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

RendererDX12::RendererDX12() : m_TextureIndex(0)
{
}

RendererDX12::~RendererDX12()
{
    OnDestroy();
}

std::string RendererDX12::GetApiDescription() const
{
    return "DirectX12";
}

// from sample https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice
static void GetHardwareAdapter(IDXGIFactory4 *factory, IDXGIAdapter1 **adapter)
{
    *adapter = nullptr;
    for (UINT adapterIndex = 0;; ++adapterIndex)
    {
        IDXGIAdapter1 *pAdapter = nullptr;
        if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &pAdapter))
        {
            break;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create
        // the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            *adapter = pAdapter;
            return;
        }
        pAdapter->Release();
    }
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

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(m_DXGIFactory.Get(), &hardwareAdapter);

    HRESULT result = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device));

    ComPtr<IDXGIAdapter> warpAdapter;
    bool useWARP = false;
    if (FAILED(result))
    {
        ThrowIfFailed(m_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));
        useWARP = true;
    }

    {
        D3D12MA::ALLOCATOR_DESC desc = {};
        desc.Flags = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS;
        desc.pDevice = m_Device.Get();
        desc.pAdapter = useWARP ? warpAdapter.Get() : hardwareAdapter.Get();

        ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &m_Allocator));
    }

    D3D12_COMMAND_QUEUE_DESC queueDescription = {};
    queueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_Device->CreateCommandQueue(&queueDescription, IID_PPV_ARGS(&m_CommandQueue)));

    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_ShaderCompiler)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_ShaderCompilerUtils)));
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

    {
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(MaxDrawCommands * sizeof(IndirectCommand));
        m_IndirectCommandHeap = CreateResource(desc, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    ThrowIfFailed(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
}

ComPtr<IDxcBlob> RendererDX12::CompileShader(const std::string &path, RageShaderType shaderType)
{
    ComPtr<IDxcBlobEncoding> source;
    ThrowIfFailed(m_ShaderCompilerUtils->LoadFile(std::wstring(path.begin(), path.end()).c_str(), nullptr, &source));

    DxcBuffer buffer{};
    buffer.Ptr = source->GetBufferPointer();
    buffer.Size = source->GetBufferSize();
    buffer.Encoding = DXC_CP_ACP;

    LPCWSTR args[] = {L"-T", L"vs_6_5", L"-E", L"VSMain"};
    switch (shaderType)
    {
    case RageShaderType::Fragment: {
        args[1] = L"ps_6_5";
        args[3] = L"PSMain";
        break;
    }
    case RageShaderType::Compute: {
        args[1] = L"cs_6_5";
        args[3] = L"CSMain";
        break;
    }
    default:
        break;
    }

    ComPtr<IDxcResult> result;
    ThrowIfFailed(m_ShaderCompiler->Compile(&buffer, args, _countof(args), nullptr, IID_PPV_ARGS(&result)));

    ComPtr<IDxcBlob> shader;
    ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));

    return shader;
}

void RendererDX12::LoadAssets(const VideoModeParams &p)
{
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0},
                                               {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0}};

        CD3DX12_ROOT_PARAMETER1 rootParams[2] = {};
        rootParams[0].InitAsConstants(0, 0);
        rootParams[1].InitAsDescriptorTable(1, &ranges[0]);

        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                           D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, flags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(
            D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_RootSignature)));
    }

    {
        D3D12_INDIRECT_ARGUMENT_DESC args[3] = {};

        // draw call
        args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

        // MatrixState index
        args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        args[1].Constant.RootParameterIndex = 0;
        args[1].Constant.DestOffsetIn32BitValues = 0;
        args[1].Constant.Num32BitValuesToSet = 1;

        // RenderState index
        args[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        args[2].Constant.RootParameterIndex = 0;
        args[2].Constant.DestOffsetIn32BitValues = 1;
        args[2].Constant.Num32BitValuesToSet = 1;

        D3D12_COMMAND_SIGNATURE_DESC desc = {};
        desc.pArgumentDescs = args;
        desc.NumArgumentDescs = _countof(args);
        desc.ByteStride = sizeof(IndirectCommand);
        desc.NodeMask = 0; // use only a single GPU? read the docs again

        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_CommandSignature;
        m_Device->CreateCommandSignature(&desc, m_RootSignature.Get(), IID_PPV_ARGS(&m_CommandSignature));
    }

    {
        // TODO: switching or SPIR-V or something later
        std::string shaderPath = FILEMAN->ResolvePath("Data/Shaders/HLSL/shaders.hlsl").substr(1);

        ComPtr<IDxcBlob> vertexShader = CompileShader(shaderPath, RageShaderType::Vertex);
        ComPtr<IDxcBlob> pixelShader = CompileShader(shaderPath, RageShaderType::Fragment);

        constexpr D3D12_INPUT_ELEMENT_DESC spriteVertexLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(RageSpriteVertex, p),
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(RageSpriteVertex, n),
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, offsetof(RageSpriteVertex, c),
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(RageSpriteVertex, t),
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        constexpr UINT layoutElementCount = _countof(spriteVertexLayout);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_RootSignature.Get();
        psoDesc.InputLayout = {spriteVertexLayout, layoutElementCount};
        psoDesc.VS = {reinterpret_cast<BYTE *>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize()};
        psoDesc.PS = {reinterpret_cast<BYTE *>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize()};
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
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
        RageSpriteVertex triangleVertices[] = {{{0.0f, 0.25f * p.fDisplayAspectRatio, 0.0f},
                                                {0.0f, 0.0f, 0.0f},
                                                RageColor(0.0f, 1.0f, 0.0f, 1.0f),
                                                {0.0f, 0.0f}},
                                               {{0.25f, -0.25f * p.fDisplayAspectRatio, 0.0f},
                                                {0.0f, 0.0f, 0.0f},
                                                RageColor(0.0f, 0.0f, 1.0f, 1.0f),
                                                {0.0f, 0.0f}},
                                               {{-0.25f, -0.25f * p.fDisplayAspectRatio, 0.0f},
                                                {0.0f, 0.0f, 0.0f},
                                                RageColor(1.0f, 0.0f, 0.0f, 1.0f),
                                                {0.0f, 0.0f}}};

        const UINT vertexBufferSize = sizeof(triangleVertices);

        auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        m_VertexBuffer = CreateResource(buffer, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

        UINT8 *pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_VertexBuffer->Unmap(0, nullptr);

        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.StrideInBytes = sizeof(RageSpriteVertex);
        m_VertexBufferView.SizeInBytes = vertexBufferSize;
    }

    {
        auto bufferProps = CD3DX12_RESOURCE_DESC::Buffer(
            Display::Display::MaxTextureSize * Display::Display::MaxTextureSize * Display::Display::TexturePixelSize);
        m_TextureUploadHeap = CreateResource(bufferProps, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
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

    const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    m_CommandList->DrawInstanced(3, 1, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_CommandList->ResourceBarrier(1, &barrier);
    ThrowIfFailed(m_CommandList->Close());
}

ComPtr<ID3D12Resource> RendererDX12::CreateResource(const D3D12_RESOURCE_DESC &resourceDesc, D3D12_HEAP_TYPE heapType,
                                                    D3D12_RESOURCE_STATES initialResourceState)
{
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = heapType;

    ComPtr<D3D12MA::Allocation> allocation;
    ThrowIfFailed(m_Allocator->CreateResource(&allocationDesc, &resourceDesc, initialResourceState, NULL, &allocation,
                                              IID_NULL, NULL));
    ComPtr<ID3D12Resource> resource = allocation->GetResource();
    return resource;
}

intptr_t RendererDX12::PushTextureCommand(const Display::TextureCommand &command)
{
    m_TextureCommandQueue.push_back(command);
    return command.index() == Display::TextureCommandType::Creation ? m_TextureIndex++ : 0;
}

void RendererDX12::SignalFence(bool waitForEvent)
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

void RendererDX12::OnRender(const ActualVideoModeParams *p, const Display::CommandBatcher &batcher)
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
