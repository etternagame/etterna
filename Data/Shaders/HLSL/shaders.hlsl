// should match Display::DrawCommandArgument
struct DrawCommandArgument
{
	uint32_t matrixStateIndex;
	uint32_t renderStateIndex;
};

// should match Display::MatrixState
struct MatrixState
{
	column_major float4x4 projection;
	column_major float4x4 view;
	column_major float4x4 world;
	column_major float4x4 texture;
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
    MatrixState m = MatrixStateBuffer[args.matrixStateIndex];
    
    float4 worldPossum = mul(m.world, float4(input.position, 1.0));
    float4 viewPossum = mul(m.view, worldPossum);
    float4 projPossum = mul(m.projection, viewPossum);
    output.pos = projPossum;
    output.pos.y *= -1.0f;

    output.color = input.color;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}