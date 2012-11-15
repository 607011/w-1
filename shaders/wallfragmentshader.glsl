// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

varying mediump vec4 vTexCoord;
uniform sampler2D uVideoTexture;
uniform sampler2D uDepthTexture;
uniform float uGamma;
uniform vec2 uOffset[9];
uniform float uSharpen[9];
uniform vec2 uSize;


void main(void)
{
    vec4 color = vec4(0.0);
    for (int i = 0; i < 9; ++i) {
        vec4 c = texture2D(uVideoTexture, vTexCoord.st + uOffset[i] / uSize);
        color += c * uSharpen[i];
    }
    gl_FragColor.rgb = pow(color.rgb, vec3(1.0 / uGamma));
}
