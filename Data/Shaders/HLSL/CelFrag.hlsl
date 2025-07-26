// for each texture unit :( (see RageDisplay.h)
sampler2D sampler0 : register(s0);
sampler2D sampler1 : register(s1);
sampler2D sampler2 : register(s2);
sampler2D sampler3 : register(s3);
sampler2D sampler4 : register(s4);
sampler2D sampler5 : register(s5);
sampler2D sampler6 : register(s6);
sampler2D sampler7 : register(s7);

int4 textureIndex : register(c0);

struct VertexShaderOutput
{
    float4 position : POSITION;
    float3 normal : TEXCOORD1;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float time : TEXCOORD2;
};

float4 main(VertexShaderOutput input) : COLOR0
{
    return input.color * tex2D(sampler0, input.texcoord);
}
