// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

varying vec4 vTexCoord;
uniform sampler2D uVideoTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uImageTexture;
uniform float uGamma;
uniform float uContrast;
uniform float uSaturation;
uniform float uSharpen[9];
uniform vec2 uOffset[9];
uniform vec2 uSize;

void main(void)
{
    vec3 color = vec3(0.0);
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
        // sharpen
        for (int i = 0; i < 9; ++i) {
            vec3 c = texture2D(uVideoTexture, vTexCoord.st + uOffset[i] / uSize).rgb;
            color += c * uSharpen[i];
        }
        // gamma
        color = pow(color, vec3(1.0 / uGamma));
        // contrast
        color = (color - 0.5) * uContrast + 0.5;
        // saturate
        float luminance = dot(color, vec3(0.2125, 0.7154, 0.0721));
        vec3 gray = vec3(luminance);
        color = mix(gray, color, uSaturation);
    }
    gl_FragColor = vec4(color, 1.0);
}
