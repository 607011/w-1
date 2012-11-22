// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

varying vec4 vTexCoord;
uniform sampler2D uImageTexture;
uniform int uFilter;

void main(void)
{
    vec3 color = texture2D(uImageTexture, vTexCoord.st).rgb;
    if (uFilter == 1) { // grayscale
        float gray = dot(color, vec3(0.299, 0.587, 0.114));
        color = vec3(gray, gray, gray);
    }
    else if (uFilter == 2) { // sepia tone
        float gray = dot(color, vec3(0.299, 0.587, 0.114));
        color = gray * vec3(1.2, 1.0, 0.8);
    }
    else if (uFilter == 3) { // negative
        color = 1.0 - color;
    }
    gl_FragColor.rgb = color;
}
