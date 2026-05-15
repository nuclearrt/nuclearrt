#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    if (texColor.a < 0.01) discard;
    FragColor = texColor * uColor;
}