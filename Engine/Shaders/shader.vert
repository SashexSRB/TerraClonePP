#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 proj;
    int useUIProj;
    int useFont;
    int useSky;
    int useLighting;
    vec2 skyUVOffset;
    vec2 skyUVScale;
    vec2 lightmapOrigin;
    vec2 lightmapSize;
    int useSprite;
    int _pad[3];
} push;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in float inZ;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec2 fragLightUV;

void main() {
    mat4 proj = (push.useUIProj == 1) ? push.proj : ubo.proj;
    mat4 view = (push.useUIProj == 1) ? mat4(1.0) : ubo.view;
    mat4 model = ubo.model;

    gl_Position = proj * view * model * vec4(inPosition, inZ, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragLightUV = (inPosition / 32.0 - push.lightmapOrigin) / push.lightmapSize;
}
