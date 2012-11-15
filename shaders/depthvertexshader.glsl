// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

attribute highp vec4 aVertex;
attribute mediump vec4 aTexCoord;
varying mediump vec4 vTexCoord;
uniform mediump mat4 uMatrix;

void main(void)
{
    gl_Position = uMatrix * aVertex;
    vTexCoord = aTexCoord;
}
