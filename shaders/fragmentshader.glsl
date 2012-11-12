// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

varying mediump vec4 texc;
uniform sampler2D texture;
uniform float gamma;
uniform vec2 offset[9];
uniform float sharpen[9];
uniform vec2 size;


void denoise(inout vec4 texel, inout vec2 coord)
{
    const float effectWidth = 0.1;
    const float val0 = 1.0;
    const float val1 = 0.125;
    float tap = effectWidth;
    vec4 accum = texel * val0;
    for (int i = 0; i < 16; i++) {
        float dx = tap / size.x;
        float dy = tap / size.y;
        accum += texture2D(texture, coord + vec2(-dx, -dy)) * val1;
        accum += texture2D(texture, coord + vec2(0.0, -dy)) * val1;
        accum += texture2D(texture, coord + vec2(-dx, 0.0)) * val1;
        accum += texture2D(texture, coord + vec2( dx, 0.0)) * val1;
        accum += texture2D(texture, coord + vec2(0.0,  dy)) * val1;
        accum += texture2D(texture, coord + vec2( dx,  dy)) * val1;
        accum += texture2D(texture, coord + vec2(-dx,  dy)) * val1;
        accum += texture2D(texture, coord + vec2( dx, -dy)) * val1;
        tap += effectWidth;
    }
    texel = accum / 16.0;
}


void main(void)
{
    vec4 color = vec4(0.0);
    for (int i = 0; i < 9; ++i) {
        vec4 c = texture2D(texture, texc.st + offset[i] / size);
        color += c * sharpen[i];
    }
    gl_FragColor.rgb = pow(color.rgb, vec3(1.0 / gamma));
}
