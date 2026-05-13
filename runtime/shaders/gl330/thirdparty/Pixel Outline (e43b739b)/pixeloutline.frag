#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform float fPixelWidth;
uniform float fPixelHeight;

uniform sampler2D uTexture;
uniform vec4 uColor;

uniform vec4 color;

vec2 dirs[] = vec2[](
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(-1.0, 0.0),
    vec2(-1.0, -1.0),
    vec2(0.0, -1.0),
    vec2(1.0, -1.0)
);

void main() {
    vec4 src = texture(uTexture, TexCoord);

    src.rgb += (color.rgb - src.rgb) * (1.0 - src.a);

    if (src.a > 0.0) {
        src.a = 1.0;
        src.rgb *= uColor.rgb;
    } else {
        for (int i = 0; i < 8; i++) {
            vec2 offset = dirs[i] * vec2(fPixelWidth, fPixelHeight);
            if (texture(uTexture, TexCoord + offset).a > 0.0) {
                src.a = 1.0;
                break;
            }
        }
    }

    FragColor = src;
}