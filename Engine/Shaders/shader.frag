#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D fontSampler;
layout(binding = 3) uniform sampler2D skySampler;
layout(binding = 4) uniform sampler2D lightSampler;
layout(binding = 5) uniform sampler2D spriteSampler;

layout(push_constant) uniform PushConstants {
    mat4 proj;
    int  useUIProj;
    int  useFont;
    int  useSky;
    int  useLighting;
    vec2 skyUVOffset;
    vec2 skyUVScale;
    vec2 lightmapOrigin;
    vec2 lightmapSize;
    int useSprite;
    int _pad[3];
} push;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 fragLightUV;
layout(location = 0) out vec4 outColor;

void main() {
    if (push.useFont == 1) {
        float dist = texture(fontSampler, fragTexCoord).r;
        float width = clamp(fwidth(dist), 0.0, 0.05);

        // Inner glyph fill
        float alpha = smoothstep(0.5 - width, 0.5 + width, dist);

        // Outline — sample a wider band below the edge
        float outlineWidth = 0.15;  // tweak this to make outline thicker/thinner
        float outline = smoothstep(0.5 - outlineWidth - width, 0.5 - outlineWidth + width, dist);

        // Composite: outline color underneath, text color on top
        vec3 outlineColor = vec3(0.0, 0.0, 0.0);
        vec3 finalColor = mix(outlineColor, fragColor.rgb, alpha);
        float finalAlpha = max(alpha, outline);

        outColor = vec4(finalColor, finalAlpha);
    } else if (push.useSky == 1) {
        vec2 uv = fragTexCoord * push.skyUVScale + push.skyUVOffset;
        outColor = texture(skySampler, uv);
    } else if (push.useSprite == 1) {
        vec4 col = texture(spriteSampler, fragTexCoord);
        if (col.a < 0.01) discard;
        outColor = col;
    } else {
        vec4 col = texture(texSampler, fragTexCoord);
        if (push.useLighting == 1) {
            vec3 light = texture(lightSampler, clamp(fragLightUV, 0.0, 1.0)).rgb;
            col.rgb *= light;
        }
        outColor = col;
    }
}
