// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

attribute highp vec4 aVertex;
attribute mediump vec4 aTexCoord;
varying mediump vec4 vTexCoord;

void main(void)
{
    gl_Position = aVertex;
    vTexCoord = vec4(1.0, 1.0, 1.0, 1.0) - aTexCoord;
}
