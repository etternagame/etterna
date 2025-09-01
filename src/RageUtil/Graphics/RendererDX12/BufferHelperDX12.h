/* Stores upload buffers for each frame of RendererDX12 */

#ifndef BUFFER_HELPER_DX12_H
#define BUFFER_HELPER_DX12_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "UtilsDX12.h"
#include <cstring>
#include <d3d12.h>
#include <directx/d3dx12.h>
#include <windows.h>

template <typename _UploadedStruct> class BufferHelperDX12
{
  public:
    BufferHelperDX12(ID3D12Device *device, UINT capacity, UINT frameCount)
        : m_Capacity(capacity), m_FrameCount(frameCount), m_Device(device), m_UploadBuffers(frameCount),
          m_CurrentState(D3D12_RESOURCE_STATE_COPY_DEST), m_FrameSize(frameCount)
    {
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(capacity * sizeof(_UploadedStruct));
        D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        for (UINT frameIndex = 0; frameIndex < m_FrameCount; frameIndex++)
        {
            ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                          IID_PPV_ARGS(&m_UploadBuffers[frameIndex])));
        }

        D3D12_RESOURCE_DESC destBufferDesc =
            CD3DX12_RESOURCE_DESC::Buffer(capacity * sizeof(_UploadedStruct), D3D12_RESOURCE_FLAG_NONE);
        D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &destBufferDesc,
                                                      D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                      IID_PPV_ARGS(&m_DestinationBuffer)));
    }

    ID3D12Resource *GetDestinationBuffer()
    {
        return m_DestinationBuffer.Get();
    }

    void CopyToDestinationBuffer(ID3D12GraphicsCommandList *commandList, UINT frameIndex)
    {
        commandList->CopyBufferRegion(m_DestinationBuffer.Get(), 0, m_UploadBuffers[frameIndex].Get(), 0,
                                      m_FrameSize[frameIndex]);
    }

    void UploadToBuffer(UINT frameIndex, const std::vector<_UploadedStruct> &inputBuffer)
    {
        void *bufferData = nullptr;
        ThrowIfFailed(m_UploadBuffers[frameIndex]->Map(0, nullptr, &bufferData));

        m_FrameSize[frameIndex] = inputBuffer.size() * sizeof(_UploadedStruct);
        std::memcpy(bufferData, inputBuffer.data(), m_FrameSize[frameIndex]);

        m_UploadBuffers[frameIndex]->Unmap(0, nullptr);
    }

    void TransitionToState(ID3D12GraphicsCommandList *commandList, D3D12_RESOURCE_STATES newState)
    {
        if (m_CurrentState == newState)
            return;

        CD3DX12_RESOURCE_BARRIER barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(m_DestinationBuffer.Get(), m_CurrentState, newState);
        commandList->ResourceBarrier(1, &barrier);
        m_CurrentState = newState;
    }

  private:
    const UINT m_FrameCount;
    const UINT m_Capacity;
    const ID3D12Device *m_Device;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DestinationBuffer;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_UploadBuffers;
    D3D12_RESOURCE_STATES m_CurrentState;
    std::vector<size_t> m_FrameSize;
};

#endif
