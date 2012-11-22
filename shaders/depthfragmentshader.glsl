// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

varying vec4 vTexCoord;
uniform sampler2D uDepthTexture;
uniform float uNearThreshold;
uniform float uFarThreshold;
uniform vec2 uSize;
uniform vec2 uHalo[%1]; // Platzhalter wird im C++-Code durch Feldgröße ersetzt
uniform vec3 uTooNearColor;
uniform vec3 uTooFarColor;
uniform vec3 uInvalidDepthColor;

const vec2 DepthFactor = vec2(65536.0, 256.0);


vec3 colorOf(in vec2 coord)
{
    for (int i = 0; i < %1/* Platzhalter wird im C++-Code ersetzt */; ++i) {
        vec2 neighbor = coord + uHalo[i] / uSize;
        float neighborDepth = dot(texture2D(uDepthTexture, neighbor).rg, DepthFactor);
        if (neighborDepth == 0.0)
            return uInvalidDepthColor;
        if (neighborDepth < uNearThreshold)
            return uTooNearColor;
        if (neighborDepth > uFarThreshold)
            return uTooFarColor;
    }
    float depth = dot(texture2D(uDepthTexture, coord).rg, DepthFactor);
    float k = (depth - uNearThreshold) / (uFarThreshold - uNearThreshold);
    return vec3(k, k, k);
}


void main(void)
{
    gl_FragColor.rgb = colorOf(vTexCoord.st);
}
