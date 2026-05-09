#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 uColor1;
uniform vec4 uColor2;
uniform int uVertical;
uniform int circleClip;

void main() {
    float t = (uVertical == 1) ? TexCoord.x : TexCoord.y;
    if (circleClip == 1) {
        float distance = length(TexCoord - vec2(0.5, 0.5));
        if (distance > 0.5) {
            discard;
        }
    }
    FragColor = mix(uColor1, uColor2, t);
}
