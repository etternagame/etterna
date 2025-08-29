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

// should match RendererDX12::IndirectCommand (kinda)
struct IndirectCommand
{
    DrawCommandArgument args;
    DrawCommand draw;
};

// should match Display::MatrixState
struct MatrixState
{
	float4x4 projection;
	float4x4 view;
	float4x4 world;
	float4x4 texture;
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

AppendStructuredBuffer<IndirectCommand> OutputCommandBuffer : register(u0);

StructuredBuffer<IndirectCommand> InputCommandBuffer : register(t0);
StructuredBuffer<MatrixState> MatrixStateBuffer : register(t1);
StructuredBuffer<RenderState> RenderStateBuffer : register(t2);
Texture2D Textures[] : register(t3);

cbuffer Constants : register(b0)
{
    uint totalCommandCount;
    uint padding;
}

// matches RendererDX12::ComputeShaderThreadCount
#define ThreadCount 64
[numthreads(ThreadCount, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * ThreadCount) + groupIndex;
    if (index >= totalCommandCount) return;

    IndirectCommand command = InputCommandBuffer[index];

    // no culling (for now)
    OutputCommandBuffer.Append(command);
}