struct VSInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    uint   color    : COLOR;
    float2 uv       : TEXCOORD;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 UnpackBGRA8(uint c)
{
    float b = (c & 0xFF) / 255.0;
    float g = ((c >> 8) & 0xFF) / 255.0;
    float r = ((c >> 16) & 0xFF) / 255.0;
    float a = ((c >> 24) & 0xFF) / 255.0;
    return float4(r, g, b, a);
}

PSInput VSMain(VSInput input)
{
    PSInput o;
    o.pos = float4(input.position, 1.0f);

    o.color = UnpackBGRA8(input.color);

    return o;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
