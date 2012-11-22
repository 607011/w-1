// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

varying vec4 vTexCoord;
uniform sampler2D uDepthTexture[%2];
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
    for (int i = 0; i < %2/* Platzhalter wird im C++-Code ersetzt */; ++i) {
        for (int j = 0; j < %1/* Platzhalter wird im C++-Code ersetzt */; ++j) {
            vec2 neighbor = coord + uHalo[j] / uSize;
            float depth = dot(texture2D(uDepthTexture[i], neighbor).rg, DepthFactor);
            if (depth == 0.0)
                return uInvalidDepthColor;
            if (depth < uNearThreshold)
                return uTooNearColor;
            if (depth > uFarThreshold)
                return uTooFarColor;
        }
    }
    float sum = 0.0;
    for (int i = 0; i < %2/* Platzhalter wird im C++-Code ersetzt */; ++i) {
        sum += dot(texture2D(uDepthTexture[i], coord).rg, DepthFactor);
    }
    float depth = sum / float(%2);
    float k = (depth - uNearThreshold) / (uFarThreshold - uNearThreshold);
    return vec3(k, k, k);
}


void main(void)
{
    gl_FragColor.rgb = colorOf(vTexCoord.st);
}
