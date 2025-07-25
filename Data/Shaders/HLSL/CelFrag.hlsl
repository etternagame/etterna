sampler2D textureSampler : register(s0);
bool useTexture : register(b0);

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
    float4 textureColor = tex2D(textureSampler, input.texcoord);
    float4 outputColor = input.color;

    if(useTexture){
        outputColor *= textureColor;
    }

    return outputColor;
}