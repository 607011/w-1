// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

attribute vec4 aVertex;
attribute vec4 aTexCoord;
varying vec4 vTexCoord;
uniform mat4 uMatrix;

void main(void)
{
    vTexCoord = aTexCoord;
    gl_Position = uMatrix * aVertex;
}
