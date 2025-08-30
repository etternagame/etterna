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

StructuredBuffer<MatrixState> MatrixStateBuffer : register(t1);
StructuredBuffer<RenderState> RenderStateBuffer : register(t2);
StructuredBuffer<DrawCommandArgument> InputCommandArgBuffer : register(t4);

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    PSInput output;
    DrawCommandArgument args = InputCommandArgBuffer[instanceID];

    MatrixState matrices = MatrixStateBuffer[args.matrixStateIndex];
    RenderState renderState = RenderStateBuffer[args.renderStateIndex];

    output.pos = float4(input.position, 1.0f);
    output.pos = mul(matrices.projection, mul(matrices.view, mul(matrices.world, output.pos)));
    output.color = input.color;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
