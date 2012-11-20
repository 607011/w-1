// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

attribute highp vec4 aVertex;
attribute highp vec4 aTexCoord;
varying highp vec4 vTexCoord;
uniform highp mat4 uMatrix;

void main(void)
{
    gl_Position = uMatrix * aVertex;
    vTexCoord = vec4(1.0, -1.0, 1.0, 1.0) * aTexCoord;
}
