// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

attribute mediump vec4 aVertex;
attribute mediump vec4 aTexCoord;
varying mediump vec4 vTexCoord;
uniform mediump mat4 uMatrix;

void main(void)
{
    vTexCoord.x = aTexCoord.x;
    vTexCoord.y = 1.0 - aTexCoord.y;
    gl_Position = uMatrix * aVertex;
}
