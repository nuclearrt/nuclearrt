#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uAlpha;
uniform int uPattern; // 0 = none, 1-3 = weft styles
uniform vec2 uSize;   // in frame pixels
uniform vec2 uReveal; // in frame pixels

bool weftVisible(vec2 p) {
    bool oddX = mod(p.x, 2.0) >= 1.0;
    bool oddY = mod(p.y, 2.0) >= 1.0;
    bool top = p.y < uReveal.y;
    bool bottom = p.y >= uSize.y - uReveal.y;
    bool left = p.x < uReveal.x;
    bool right = p.x >= uSize.x - uReveal.x;

    if (uPattern == 1) return oddX ? bottom : top;
    if (uPattern == 2) return oddX ? left : right;
    if (oddX == oddY) return oddX ? bottom : top;
    return oddX ? left : right;
}

void main() {
    // render target textures are stored bottom-up
    if (uPattern != 0 && !weftVisible(floor(vec2(TexCoord.x, 1.0 - TexCoord.y) * uSize))) {
        discard;
    }

    vec4 color = texture(uTexture, TexCoord);
    FragColor = vec4(color.rgb, color.a * uAlpha);
}
