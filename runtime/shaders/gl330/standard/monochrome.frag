#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    texColor *= uColor;
    if (texColor.a < 0.01) discard;
    float gray = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(vec3(gray), texColor.a);
}
