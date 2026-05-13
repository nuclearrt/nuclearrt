#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform float fPixelWidth;
uniform float fPixelHeight;

uniform sampler2D uTexture;
uniform vec4 uColor;

uniform float Rx, Ry;
uniform float Gx, Gy;
uniform float Bx, By;
uniform float Ax, Ay;

vec4 texClip(sampler2D tex, vec2 pos)
{
    if (pos.x < 0.0 || pos.y < 0.0 || pos.x > 1.0 || pos.y > 1.0)
        return vec4(0.0);
    return texture(tex, pos);
}

void main() {
    vec4 color = texture(uTexture, TexCoord);
    vec2 Pixel = vec2(fPixelWidth, fPixelHeight);

    color.r = texClip(uTexture, TexCoord + Pixel * vec2(Rx, Ry)).r;
    color.g = texClip(uTexture, TexCoord + Pixel * vec2(Gx, Gy)).g;
    color.b = texClip(uTexture, TexCoord + Pixel * vec2(Bx, By)).b;
    color.a = texClip(uTexture, TexCoord + Pixel * vec2(Ax, Ay)).a;

    FragColor = color;
}
