#include "RendererDX12.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Graphics/RageSurface.h"
#include "RageUtil/Graphics/RendererDX12//UtilsDX12.h"
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
        if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
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

    D3D12_COMMAND_QUEUE_DESC graphicsQueueDesc = {};
    graphicsQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    graphicsQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_Device->CreateCommandQueue(&graphicsQueueDesc, IID_PPV_ARGS(&m_GraphicsHelpers.CommandQueue)));

    D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
    computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

    ThrowIfFailed(m_Device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&m_ComputeHelpers.CommandQueue)));

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
    ThrowIfFailed(
        m_DXGIFactory->CreateSwapChain(m_GraphicsHelpers.CommandQueue.Get(), &swapChainDescription, &swapChain));
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
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(MaxDrawCommands * sizeof(Display::DrawCommand));
        m_IndirectCommandHeap = CreateResource(desc, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = MintyFreshDescriptorCount;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_MintyFreshHeap)));

        m_MintyFreshDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_MintyFreshHeapCpuHandle = m_MintyFreshHeap->GetCPUDescriptorHandleForHeapStart();
        m_MintyFreshHeapGpuHandle = m_MintyFreshHeap->GetGPUDescriptorHandleForHeapStart();
    }

    ThrowIfFailed(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                   IID_PPV_ARGS(&m_GraphicsHelpers.CommandAllocator)));

    ThrowIfFailed(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                   IID_PPV_ARGS(&m_ComputeHelpers.CommandAllocator)));

    InitUploadBufferHelpers();
    CreateViewsForBufferHelpers();
}

ComPtr<IDxcBlob> RendererDX12::CompileShader(const std::string &path, RageShaderType shaderType)
{
    ComPtr<IDxcBlobEncoding> source;
    ThrowIfFailed(m_ShaderCompilerUtils->LoadFile(std::wstring(path.begin(), path.end()).c_str(), nullptr, &source));

    DxcBuffer buffer{};
    buffer.Ptr = source->GetBufferPointer();
    buffer.Size = source->GetBufferSize();
    buffer.Encoding = DXC_CP_ACP;

    LPCWSTR args[] = {L"-T", L"", L"-E", L""};
    switch (shaderType)
    {
    case RageShaderType::Fragment: {
        args[1] = L"ps_6_5";
        args[3] = L"PSMain";
        break;
    }
    case RageShaderType::Vertex: {
        args[1] = L"vs_6_5";
        args[3] = L"VSMain";
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
        CD3DX12_DESCRIPTOR_RANGE1 ranges[3] = {};

        // InputCommand stuff
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
        // OutputCommandBuffer
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
        // Textures (future proofing or smth idk)
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
        // DrawCommandArgument
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER1 params[4] = {};

        params[0].InitAsDescriptorTable(1, &ranges[0]);
        params[1].InitAsDescriptorTable(1, &ranges[1]);
        params[2].InitAsDescriptorTable(1, &ranges[2]);
        params[3].InitAsDescriptorTable(1, &ranges[3]);

        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
        desc.Init_1_1(_countof(params), params, 0, nullptr, flags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        auto result = D3D12SerializeVersionedRootSignature(&desc, &signature, &error);
        if (FAILED(result))
        {
            std::string msg = (const char *)error->GetBufferPointer();
            Locator::getLogger()->error(msg);
            ThrowIfFailed(result);
        }

        ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_RootSignature)));
    }

    {
        D3D12_INDIRECT_ARGUMENT_DESC args[1] = {};

        // Draw call (must come last due to DirectX something something)
        args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

        D3D12_COMMAND_SIGNATURE_DESC desc = {};
        desc.pArgumentDescs = args;
        desc.NumArgumentDescs = _countof(args);
        desc.ByteStride = sizeof(Display::DrawCommand);
        desc.NodeMask = 0;

        ThrowIfFailed(
            m_Device->CreateCommandSignature(&desc, m_RootSignature.Get(), IID_PPV_ARGS(&m_IndirectCommandSignature)));
    }

    {
        D3D12_RESOURCE_DESC outputBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		  MaxDrawCommands * sizeof(Display::DrawCommand),
		  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        m_OutputCommandBuffer =
            CreateResource(outputBufferDesc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = MaxDrawCommands;
		uavDesc.Buffer.StructureByteStride = sizeof(Display::DrawCommand);
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = {m_MintyFreshHeapCpuHandle.ptr +
                                                 DescriptorHeapOffsets::OutputCommandUav * m_MintyFreshDescriptorSize};

        m_Device->CreateUnorderedAccessView(m_OutputCommandBuffer.Get(), m_OutputCommandBuffer.Get(), &uavDesc,
                                            uavHandle);
    }

    {
        std::string shaderPath = FILEMAN->ResolvePath("Data/Shaders/HLSL/IndirectCommand.hlsl").substr(1);
        m_IndirectCommandShader = CompileShader(shaderPath, RageShaderType::Compute);

        D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
        computePsoDesc.pRootSignature = m_RootSignature.Get();
        computePsoDesc.CS = {reinterpret_cast<BYTE *>(m_IndirectCommandShader->GetBufferPointer()),
                             m_IndirectCommandShader->GetBufferSize()};
        ThrowIfFailed(
            m_Device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_ComputeHelpers.PipelineState)));
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
        ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_GraphicsHelpers.PipelineState)));
    }

    ThrowIfFailed(m_Device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_GraphicsHelpers.CommandAllocator.Get(),
        m_GraphicsHelpers.PipelineState.Get(), IID_PPV_ARGS(&m_GraphicsHelpers.CommandList)));

    ThrowIfFailed(m_GraphicsHelpers.CommandList->Close());

    ThrowIfFailed(
        m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_ComputeHelpers.CommandAllocator.Get(),
                                    m_ComputeHelpers.PipelineState.Get(), IID_PPV_ARGS(&m_ComputeHelpers.CommandList)));

    ThrowIfFailed(m_ComputeHelpers.CommandList->Close());

    {
        auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(RageSpriteVertex) * MaxVertices);
        m_VertexBuffer = CreateResource(buffer, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
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

        WaitForGPU();
    }
}

void RendererDX12::PopulateCommandList(const ActualVideoModeParams *p)
{
    ThrowIfFailed(m_GraphicsHelpers.CommandAllocator->Reset());
    ThrowIfFailed(m_GraphicsHelpers.CommandList->Reset(m_GraphicsHelpers.CommandAllocator.Get(),
                                                       m_GraphicsHelpers.PipelineState.Get()));

    m_GraphicsHelpers.CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

    m_Viewport = D3D12_VIEWPORT(0.0f, 0.0f, static_cast<float>(p->width), static_cast<float>(p->height));
    m_ScissorRect = D3D12_RECT(0, 0, static_cast<LONG>(p->width), static_cast<LONG>(p->height));
    m_GraphicsHelpers.CommandList->RSSetViewports(1, &m_Viewport);
    m_GraphicsHelpers.CommandList->RSSetScissorRects(1, &m_ScissorRect);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_RenderTargets[m_FrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_GraphicsHelpers.CommandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex,
                                            m_RtvDescriptorSize);
    m_GraphicsHelpers.CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_GraphicsHelpers.CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    m_GraphicsHelpers.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_GraphicsHelpers.CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

    ID3D12DescriptorHeap *ppHeaps[] = {m_MintyFreshHeap.Get()};
    m_GraphicsHelpers.CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_MintyFreshHeap->GetGPUDescriptorHandleForHeapStart();

    uint32_t stuffs[2] = {};
    m_GraphicsHelpers.CommandList->SetGraphicsRoot32BitConstants(0, 2, stuffs, 0);

    m_GraphicsHelpers.CommandList->SetGraphicsRootDescriptorTable(1, gpuHandle);

    D3D12_GPU_DESCRIPTOR_HANDLE outputUavHandle = gpuHandle;
    outputUavHandle.ptr += DescriptorHeapOffsets::OutputCommandUav * m_MintyFreshDescriptorSize;
    m_GraphicsHelpers.CommandList->SetGraphicsRootDescriptorTable(2, outputUavHandle);

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle = gpuHandle;
    textureSrvHandle.ptr += DescriptorHeapOffsets::TextureSrv * m_MintyFreshDescriptorSize;
    m_GraphicsHelpers.CommandList->SetGraphicsRootDescriptorTable(3, textureSrvHandle);

    m_GraphicsHelpers.CommandList->ExecuteIndirect(m_IndirectCommandSignature.Get(), MaxDrawCommands,
                                                   m_OutputCommandBuffer.Get(), 0, nullptr, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_GraphicsHelpers.CommandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(m_GraphicsHelpers.CommandList->Close());

    m_FenceValue++;
}

void RendererDX12::RunIndirectCommandShader(const Display::CommandBatcher &batcher)
{
    UploadBatchToBufferHelpers(batcher);

    ThrowIfFailed(m_ComputeHelpers.CommandAllocator->Reset());
    ThrowIfFailed(m_ComputeHelpers.CommandList->Reset(m_ComputeHelpers.CommandAllocator.Get(),
                                                      m_ComputeHelpers.PipelineState.Get()));

    m_ComputeHelpers.CommandList->SetComputeRootSignature(m_RootSignature.Get());

    ID3D12DescriptorHeap *ppHeaps[] = {m_MintyFreshHeap.Get()};
    m_ComputeHelpers.CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    const auto indirectCommandCount = batcher.m_IndirectCommandBuffer.size();

    // note to self: maybe transition all helpers at the same time instead of whatever this is?
    m_IndirectCommandHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
    m_MatrixStateHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
    m_RenderStateHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
    m_RageSpriteVertexHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
    CopyHelperDataToDestBuffers();
    m_IndirectCommandHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(),
                                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    m_MatrixStateHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(),
                                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    m_RenderStateHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(),
                                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    m_RageSpriteVertexHelper->TransitionToState(m_ComputeHelpers.CommandList.Get(),
                                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    // InputCommand, MatrixState, RenderState
    m_ComputeHelpers.CommandList->SetComputeRootDescriptorTable(1, m_MintyFreshHeapGpuHandle);

    // OutputCommandBuffer
    D3D12_GPU_DESCRIPTOR_HANDLE outputUavHandle = m_MintyFreshHeapGpuHandle;
    outputUavHandle.ptr += DescriptorHeapOffsets::OutputCommandUav * m_MintyFreshDescriptorSize;
    m_ComputeHelpers.CommandList->SetComputeRootDescriptorTable(2, outputUavHandle);

    // Textures
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle = m_MintyFreshHeapGpuHandle;
    textureSrvHandle.ptr += DescriptorHeapOffsets::TextureSrv * m_MintyFreshDescriptorSize;
    m_ComputeHelpers.CommandList->SetComputeRootDescriptorTable(3, textureSrvHandle);

    if (indirectCommandCount > 0)
    {
        UINT dispatchGroupCount = (indirectCommandCount + ComputeShaderThreadCount - 1) / ComputeShaderThreadCount;
        m_ComputeHelpers.CommandList->Dispatch(dispatchGroupCount, 1, 1);
    }

    ThrowIfFailed(m_ComputeHelpers.CommandList->Close());

    ID3D12CommandList *ppCommandLists[] = {m_ComputeHelpers.CommandList.Get()};
    m_ComputeHelpers.CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ThrowIfFailed(m_ComputeHelpers.CommandQueue->Signal(m_Fence.Get(), m_FenceValue));
    ThrowIfFailed(m_GraphicsHelpers.CommandQueue->Wait(m_Fence.Get(), m_FenceValue));

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
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

void RendererDX12::WaitForGPU()
{
    const uint64_t fence = m_FenceValue;
    ThrowIfFailed(m_GraphicsHelpers.CommandQueue->Signal(m_Fence.Get(), fence));
    m_FenceValue++;

    if (m_Fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void RendererDX12::OnRender(const ActualVideoModeParams *p, const Display::CommandBatcher &batcher)
{
    RunIndirectCommandShader(batcher);
    PopulateCommandList(p);

    ID3D12CommandList *CommandLists[] = {m_GraphicsHelpers.CommandList.Get()};
    m_GraphicsHelpers.CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

    ThrowIfFailed(m_SwapChain->Present(1, 0));
    WaitForGPU();
}

void RendererDX12::OnDestroy()
{
    WaitForGPU();
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

void RendererDX12::InitUploadBufferHelpers()
{
	m_IndirectCommandHelper =
	  std::make_unique<BufferHelperDX12<Display::DrawCommand>>(
        m_Device.Get(), MaxDrawCommands, Display::Display::FrameCount);
	m_IndirectCommandArgumentHelper =
	  std::make_unique<BufferHelperDX12<Display::DrawCommandArgument>>(
        m_Device.Get(), MaxDrawCommands, Display::Display::FrameCount);
    m_RageSpriteVertexHelper =
        std::make_unique<BufferHelperDX12<RageSpriteVertex>>(m_Device.Get(), MaxVertices, Display::Display::FrameCount);
    m_RenderStateHelper = std::make_unique<BufferHelperDX12<Display::RenderState>>(m_Device.Get(), MaxDrawCommands,
                                                                                   Display::Display::FrameCount);
    m_MatrixStateHelper = std::make_unique<BufferHelperDX12<Display::MatrixState>>(m_Device.Get(), MaxDrawCommands,
                                                                                   Display::Display::FrameCount);
}

void RendererDX12::UploadBatchToBufferHelpers(const Display::CommandBatcher &batcher)
{
    m_IndirectCommandHelper->UploadToBuffer(m_FrameIndex, batcher.m_IndirectCommandBuffer);
    m_IndirectCommandArgumentHelper->UploadToBuffer(m_FrameIndex, batcher.m_IndirectCommandArgumentBuffer);
    m_RageSpriteVertexHelper->UploadToBuffer(m_FrameIndex, batcher.m_SpriteVertexBuffer);
    m_RenderStateHelper->UploadToBuffer(m_FrameIndex, batcher.m_RenderStateBuffer);
    m_MatrixStateHelper->UploadToBuffer(m_FrameIndex, batcher.m_MatrixStateBuffer);
}

void RendererDX12::CopyHelperDataToDestBuffers()
{
    m_IndirectCommandHelper->CopyToDestinationBuffer(m_ComputeHelpers.CommandList.Get(), m_FrameIndex);
	m_IndirectCommandArgumentHelper->CopyToDestinationBuffer(
	  m_ComputeHelpers.CommandList.Get(), m_FrameIndex);
    m_RageSpriteVertexHelper->CopyToDestinationBuffer(m_ComputeHelpers.CommandList.Get(), m_FrameIndex);
    m_RenderStateHelper->CopyToDestinationBuffer(m_ComputeHelpers.CommandList.Get(), m_FrameIndex);
    m_MatrixStateHelper->CopyToDestinationBuffer(m_ComputeHelpers.CommandList.Get(), m_FrameIndex);
}

void RendererDX12::CreateViewsForBufferHelpers()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC indirectCommandSrvDesc = {};
    indirectCommandSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    indirectCommandSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    indirectCommandSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    indirectCommandSrvDesc.Buffer.FirstElement = 0;
    indirectCommandSrvDesc.Buffer.NumElements = static_cast<UINT>(MaxDrawCommands);
	indirectCommandSrvDesc.Buffer.StructureByteStride =
	  sizeof(Display::DrawCommand);
    indirectCommandSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    m_Device->CreateShaderResourceView(
        m_IndirectCommandHelper->GetDestinationBuffer(), &indirectCommandSrvDesc,
        {m_MintyFreshHeapCpuHandle.ptr + DescriptorHeapOffsets::IndirectCommandSrv * m_MintyFreshDescriptorSize});

    D3D12_SHADER_RESOURCE_VIEW_DESC indirectCommandArgSrvDesc = {};
	indirectCommandArgSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	indirectCommandArgSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	indirectCommandArgSrvDesc.Shader4ComponentMapping =
	  D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	indirectCommandArgSrvDesc.Buffer.FirstElement = 0;
	indirectCommandArgSrvDesc.Buffer.NumElements =
	  static_cast<UINT>(MaxDrawCommands);
	indirectCommandArgSrvDesc.Buffer.StructureByteStride =
	  sizeof(Display::DrawCommand);
	indirectCommandArgSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    m_Device->CreateShaderResourceView(
	  m_IndirectCommandArgumentHelper->GetDestinationBuffer(),
	  &indirectCommandArgSrvDesc,
        {m_MintyFreshHeapCpuHandle.ptr + DescriptorHeapOffsets::IndirectCommandArgSrv * m_MintyFreshDescriptorSize});

    D3D12_SHADER_RESOURCE_VIEW_DESC matrixSrvDesc = {};
    matrixSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    matrixSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    matrixSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    matrixSrvDesc.Buffer.FirstElement = 0;
    matrixSrvDesc.Buffer.NumElements = static_cast<UINT>(MaxDrawCommands);
    matrixSrvDesc.Buffer.StructureByteStride = sizeof(Display::MatrixState);
    matrixSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    m_Device->CreateShaderResourceView(
        m_MatrixStateHelper->GetDestinationBuffer(), &matrixSrvDesc,
        {m_MintyFreshHeapCpuHandle.ptr + DescriptorHeapOffsets::MatrixStateSrv * m_MintyFreshDescriptorSize});

    D3D12_SHADER_RESOURCE_VIEW_DESC renderSrvDesc = {};
    renderSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    renderSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    renderSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    renderSrvDesc.Buffer.FirstElement = 0;
    renderSrvDesc.Buffer.NumElements = static_cast<UINT>(MaxDrawCommands);
    renderSrvDesc.Buffer.StructureByteStride = sizeof(Display::RenderState);
    renderSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    m_Device->CreateShaderResourceView(
        m_RenderStateHelper->GetDestinationBuffer(), &renderSrvDesc,
        {m_MintyFreshHeapCpuHandle.ptr + DescriptorHeapOffsets::RenderStateSrv * m_MintyFreshDescriptorSize});

    m_VertexBufferView.BufferLocation = m_RageSpriteVertexHelper->GetDestinationBuffer()->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes = static_cast<UINT>(MaxVertices * sizeof(RageSpriteVertex));
    m_VertexBufferView.StrideInBytes = sizeof(RageSpriteVertex);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = MaxDrawCommands;
	uavDesc.Buffer.StructureByteStride = sizeof(Display::DrawCommand);
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    m_Device->CreateUnorderedAccessView(
        m_OutputCommandBuffer.Get(), nullptr, &uavDesc,
        {m_MintyFreshHeapCpuHandle.ptr + DescriptorHeapOffsets::OutputCommandUav * m_MintyFreshDescriptorSize});

    D3D12_SHADER_RESOURCE_VIEW_DESC textureSrvDesc = {};
    textureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    textureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    textureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureSrvDesc.Texture2D.MipLevels = 1;

    m_Device->CreateShaderResourceView(
        nullptr, &textureSrvDesc,
        {m_MintyFreshHeapCpuHandle.ptr + DescriptorHeapOffsets::TextureSrv * m_MintyFreshDescriptorSize});
}
