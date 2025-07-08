// TODO: port Cel.frag :-)
sampler2D textureSampler : register(s0);

struct VertexShaderOutput
{
    float4 pos : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

float4 main(VertexShaderOutput input) : COLOR0
{
    // ?
    float4 textureColor = tex2D(textureSampler, input.texcoord);
    float alpha = textureColor.a;
    float3 rgb = input.color.rgb;
    return float4(rgb, alpha);
}