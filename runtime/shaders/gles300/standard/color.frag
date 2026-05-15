#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 uColor;
uniform int circleClip;

void main() {
    if (circleClip == 1) {
        float distance = length(TexCoord - vec2(0.5, 0.5));
        if (distance > 0.5) {
            discard;
        }
    }
    FragColor = uColor;
}