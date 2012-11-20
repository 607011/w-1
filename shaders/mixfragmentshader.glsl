// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

varying mediump vec4 vTexCoord;
uniform sampler2D uVideoTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uImageTexture;
uniform float uGamma;
uniform mediump vec2 uOffset[9];
uniform float uSharpen[9];
uniform mediump vec2 uSize;

void main(void)
{
    vec3 color = 0.0;
    vec3 invisible = texture2D(uImageTexture, vTexCoord.st).bgr;
    if (all(equal(texture2D(uDepthTexture, vTexCoord.st).bgr, vec3(0.0, 251.0 / 255.0, 190.0 / 255.0)))) {
        color = invisible;
    }
    else if (all(equal(texture2D(uDepthTexture, vTexCoord.st).bgr, vec3(251.0 / 255.0, 85.0 / 255.0, 5.0 / 255.0)))) {
        color = invisible;
    }
    else if (all(equal(texture2D(uDepthTexture, vTexCoord.st).bgr, vec3(8.0 / 255.0, 98.0 / 255.0, 250.0 / 255.0)))) {
        color = invisible;
    }
    else {
        for (int i = 0; i < 9; ++i) {
            vec3 c = texture2D(uVideoTexture, vTexCoord.st + uOffset[i] / uSize).rgb;
            color += c * uSharpen[i];
        }
        color = pow(color, vec3(1.0 / uGamma));
    }
    gl_FragColor = vec4(color, 1.0);
}
