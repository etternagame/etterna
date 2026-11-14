#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_samplerless_texture_functions : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(set = 1, binding = 2) uniform texture2D textures[];
layout(set = 1, binding = 3) uniform sampler samplers[];

layout(location = 0) in vec4 vertexColor;
layout(location = 1) flat in uint textureIndex;
layout (location = 2) flat in uint samplerIndex;
layout(location = 3) in vec2 vertexUV;

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

layout(location = 0) out vec4 fragmentColor;

void main() {
    if (vertexColor.a <= 1.0 / 256.0) {
        discard;
    }
    if(textureIndex == 0){
        fragmentColor = vertexColor;
        return;
    }

    vec4 textureColor = texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(samplerIndex)]), vertexUV);
    fragmentColor = vertexColor * textureColor;
    if (fragmentColor.a <= 1.0 / 256.0) {
        discard;
    }
}