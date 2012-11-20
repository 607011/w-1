// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

varying mediump vec4 vTexCoord;
uniform sampler2D uImageTexture;

void main(void)
{
    gl_FragColor.rgb = texture2D(uImageTexture, vTexCoord.st).rgb;
}
