#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform int circleClip;
uniform vec2 uTileScale;

void main() {
    if (circleClip == 1) {
        float distance = length(TexCoord - vec2(0.5, 0.5));
        if (distance > 0.5) {
            discard;
        }
    }
    
    vec2 uv = fract(TexCoord * uTileScale);
    vec4 texColor = texture(uTexture, uv);
    if (texColor.a < 0.01) discard;
    FragColor = texColor * uColor;
}