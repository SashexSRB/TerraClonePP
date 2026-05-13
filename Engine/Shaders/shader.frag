#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D fontSampler;

layout(push_constant) uniform PushConstants {
    mat4 proj;
    int  useUIProj;
    int  useFont;
    int  _pad[2];
} push;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    if (push.useFont == 1) {
        vec2 fontSample = texture(fontSampler, fragTexCoord).rg;
        float outline = fontSample.r;
        float glyph   = fontSample.g;

        // Outline is dark, glyph is the text color
        vec3 outlineColor = vec3(0.0, 0.0, 0.0);
        vec4 outlinePixel = vec4(outlineColor, outline);
        vec4 glyphPixel   = vec4(fragColor, glyph);

        // Blend — glyph on top of outline
        outColor = mix(outlinePixel, glyphPixel, glyph);
    } else {
        outColor = texture(texSampler, fragTexCoord);
    }
}
