// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

varying mediump vec4 vTexCoord;
uniform sampler2D uDepthTexture;
uniform vec2 uSize;
uniform float uNearThreshold;
uniform float uFarThreshold;

void main(void)
{
    vec4 color = texture2D(uVideoTexture, vTexCoord.st);
    gl_FragColor.rgb = color.rgb;
}
