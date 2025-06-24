float4x4 worldViewProj : register(c0);

struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
};

struct VertexShaderOutput
{
    float4 pos : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.pos = mul(worldViewProj, float4(input.position, 1.0));
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}