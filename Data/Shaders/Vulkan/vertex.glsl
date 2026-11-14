#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct Vertex {
    float pos[3];
    float normal[3];
    uint color;
    float uv[2];
    uint MatrixIndex;
	uint TextureIndex;
	uint SamplerIndex;
};

layout(std430, set = 0, binding = 0) readonly buffer VertexBuffer {
    Vertex vertices[];
};

struct MatrixState {
    mat4 wvp;
    mat4 texture;
};

layout(std430, set = 0, binding = 1) readonly buffer MatrixStateBuffer {
    MatrixState matrices[];
};

layout(location = 0) out vec4 vertexColor;
layout(location = 1) out uint textureIndex;
layout(location = 2) out uint samplerIndex;
layout(location = 3) out vec2 vertexUV;

layout(buffer_reference, scalar, buffer_reference_align = 8) readonly buffer VertexShaderArgs {
    int coolInt;
};

layout(buffer_reference, scalar, buffer_reference_align = 8) readonly buffer FragmentShaderArgs {
    float coolFloat;
};

layout(push_constant) uniform PushConstants {
    VertexShaderArgs vertexArgs;
    FragmentShaderArgs fragmentArgs;
} __shaderArgs;

vec2 unpackVec2(float array[2]){
    return vec2(array[0], array[1]);
}

vec3 unpackVec3(float array[3]){
    return vec3(array[0], array[1], array[2]);
}

vec4 unpackColor(uint c)
{
    float b = float(c & 0xFFu);
    float g = float((c >> 8) & 0xFFu);
    float r = float((c >> 16) & 0xFFu);
    float a = float((c >> 24) & 0xFFu);

    return vec4(r, g, b, a) / 255.0;
}

void main() {
    Vertex currentVertex = vertices[gl_VertexIndex];
    
    vertexColor = unpackColor(currentVertex.color);
    
    textureIndex = currentVertex.TextureIndex;
    samplerIndex = currentVertex.SamplerIndex;

    uint matrixIndex = currentVertex.MatrixIndex;
    mat4 wvp = matrices[matrixIndex].wvp;
    mat4 tex = matrices[matrixIndex].texture;
    
    vec4 pos = wvp * vec4(unpackVec3(currentVertex.pos), 1.0);
    pos.z = (pos.w - pos.z) * 0.5; // [-1; 1] -> [0; 1] because reasons
    
    gl_Position = pos;

    vertexUV = unpackVec2(currentVertex.uv);
    vertexUV.x += tex[3][0];
    vertexUV.y += tex[3][1];
}