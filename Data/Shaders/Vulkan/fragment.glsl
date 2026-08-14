#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(set = 1, binding = 2) uniform texture2D textures[];
layout(set = 1, binding = 3) uniform sampler samplers[];

layout(location = 0) in vec4 vertexColor;
layout(location = 1) flat in uint textureIndex;
layout (location = 2) flat in uint samplerIndex;
layout(location = 3) in vec2 vertexUV;

layout(location = 0) out vec4 fragmentColor;

void main() {
    if(textureIndex == 0){
        fragmentColor = vertexColor;
        return;
    }

    vec4 textureColor = texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(samplerIndex)]), vertexUV);
    fragmentColor = vertexColor * textureColor;
}