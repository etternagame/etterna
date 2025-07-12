float4x4 worldViewProj : register(c0);
float Time : register(c4);

struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
};

struct VertexShaderOutput
{
    float4 position : POSITION;
    float3 normal : TEXCOORD1;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float time : TEXCOORD2;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));

    output.texcoord = input.texcoord;
    output.color = input.color;

    output.normal = mul((float3x3)worldViewProj, input.normal);
    output.normal = normalize(output.normal);

    output.time = Time;

    return output;
}