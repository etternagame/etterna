// TODO: port Cel.frag :-)

struct VertexShaderOutput
{
    float4 pos : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

float4 main(VertexShaderOutput input) : COLOR0
{
    return input.color;
}