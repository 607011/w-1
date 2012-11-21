// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

varying vec4 vTexCoord;
uniform sampler2D uDepthTexture;
uniform float uNearThreshold;
uniform float uFarThreshold;
uniform vec2 uSize;
uniform float uNeighborhoodSize;

const vec3 TooNearColor = vec3(251.0 / 255.0, 85.0 / 255.0, 5.0 / 255.0);
const vec3 TooFarColor = vec3(8.0 / 255.0, 98.0 / 255.0, 250.0 / 255.0);
const vec3 InvalidDepthColor = vec3(0.0, 251.0 / 255.0, 190.0 / 255.0);
const vec2 DepthFactor = vec2(65536.0, 256.0);


vec3 colorOf(in vec2 coord)
{
    for (float dx = -uNeighborhoodSize; dx < uNeighborhoodSize; dx += 1.0) {
        for (float dy = -uNeighborhoodSize; dy < uNeighborhoodSize; dy += 1.0) {
            vec2 neighbor = vTexCoord.st + vec2(dx, dy) / uSize;
            float neighborDepth = dot(texture2D(uDepthTexture, neighbor).rg, DepthFactor);
            if (neighborDepth == 0.0)
                return InvalidDepthColor;
            if (neighborDepth < uNearThreshold)
                return TooNearColor;
            if (neighborDepth > uFarThreshold)
                return TooFarColor;
        }
    }
    float depth = dot(texture2D(uDepthTexture, coord).rg, DepthFactor);
    float k = (depth - uNearThreshold) / (uFarThreshold - uNearThreshold);
    return vec3(k, k, k);
}


void main(void)
{
    gl_FragColor.rgb = colorOf(vTexCoord.st);
}
