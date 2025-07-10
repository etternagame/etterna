sampler2D textureSampler : register(s0);

struct VertexShaderOutput
{
    float4 position : POSITION;
    float3 normal : TEXCOORD1;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
};

float4 main(VertexShaderOutput input) : COLOR0
{
    float4 textureColor = tex2D(textureSampler, input.texcoord);
    return textureColor * input.color;
}