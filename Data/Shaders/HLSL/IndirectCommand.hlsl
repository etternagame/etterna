// should match Display::DrawCommand
struct DrawCommand
{
    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

// should match Display::DrawCommandArgument
struct DrawCommandArgument
{
	uint32_t matrixStateIndex;
	uint32_t renderStateIndex;
};

// should match Display::MatrixState
struct MatrixState
{
	float4x4 projection;
	float4x4 view;
	float4x4 world;
	float4x4 texture;
};

cbuffer CommandCount : register(b0)
{
    uint IndirectCommandCount;
};

// should match Display::RenderState
#define NUM_TextureUnit 8
struct RenderState
{
    uint cullMode;
    uint zTestMode;
    uint blendMode;
    float zBias;
    bool zWrite;
    bool alphaTest;
    bool textureWrapping[NUM_TextureUnit];
    bool textureFiltering[NUM_TextureUnit];
    uint textureMode[NUM_TextureUnit];
    uint64_t textures[NUM_TextureUnit];
};

RWStructuredBuffer<DrawCommand> OutputCommandBuffer : register(u0);

StructuredBuffer<DrawCommand> InputCommandBuffer : register(t0);
StructuredBuffer<MatrixState> MatrixStateBuffer : register(t1);
StructuredBuffer<RenderState> RenderStateBuffer : register(t2);
Texture2D Textures[] : register(t3);
StructuredBuffer<DrawCommandArgument> InputCommandArgBuffer : register(t4);

// matches RendererDX12::ComputeShaderThreadCount
#define ThreadCount 64
[numthreads(ThreadCount, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * ThreadCount) + groupIndex;
    if (index >= IndirectCommandCount) {
        OutputCommandBuffer[index].VertexCountPerInstance = 0;
        OutputCommandBuffer[index].InstanceCount = 0;
        OutputCommandBuffer[index].StartVertexLocation = 0;
        OutputCommandBuffer[index].StartInstanceLocation = 0;
    }
    DrawCommand command = InputCommandBuffer[index];

    // no culling (for now)
    OutputCommandBuffer[index] = command;
}