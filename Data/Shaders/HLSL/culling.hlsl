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
    DrawCommand draw;
    DrawCommandArgument args;
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

StructuredBuffer<IndirectCommand> InputCommandBuffer : register(t0);
RWStructuredBuffer<IndirectCommand> OutputCommandBuffer : register(u0);

StructuredBuffer<MatrixState> MatrixStateBuffer : register(t1);
StructuredBuffer<RenderState> RenderStateBuffer : register(t2);

#define ThreadCount 128
#define TotalCommandCount 100000

[numthreads(ThreadCount, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= TotalCommandCount) return;

    IndirectCommand command = InputCommandBuffer[id.x];

    MatrixState m = MatrixStateBuffer[command.args.matrixStateIndex];

    // TODO: cull the things based on RenderState+MatrixState?
    //if (IsVisible(???))
    //{
        uint outputIndex = OutputCommandBuffer.IncrementCounter();
        OutputCommandBuffer[outputIndex] = command;
    //}
}

// ----------------------------------
// I wonder if you know
// How they live in Tokyo (Hai)
// If you seen it, then you mean it
// Then you know you have to go
// Fast & Furious
// (Kita~~, drift, drift, drift)
// Fast & Furious
// (Kita~~, drift, drift, drift)
//
// (teriyaki boyz -- tokyo drift btw)
// ----------------------------------