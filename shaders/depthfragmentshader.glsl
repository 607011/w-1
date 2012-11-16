// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

varying mediump vec4 vTexCoord;
uniform sampler2D uDepthTexture;
uniform float uNearThreshold;
uniform float uFarThreshold;

void main(void)
{
    vec2 RGB = texture2D(uDepthTexture, vTexCoord.st).rg;
    float depth = RGB.r * 65536.0 + RGB.g * 256.0;
    vec3 color;
    if (depth == 0.0) {
        color = vec3(0.0, 251.0 / 255.0, 190.0 / 255.0);
    }
    else if (depth < uNearThreshold) {
        color = vec3(251.0 / 255.0, 85.0 / 255.0, 5.0 / 255.0);
    }
    else if (depth > uFarThreshold) {
        color = vec3(8.0 / 255.0, 98.0 / 255.0, 250.0 / 255.0);
    }
    else {
        float k = (depth - uNearThreshold) / (uFarThreshold - uNearThreshold);
        color = vec3(k, k, k);
    }
    gl_FragColor = vec4(color, 1.0);
}
