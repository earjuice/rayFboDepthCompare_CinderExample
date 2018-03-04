#version 150

uniform mat4 ciModelViewProjection;
in vec4 ciPosition;
in vec4 ciColor;
in vec2 ciTexCoord0;

out highp vec2 fragCoord;
out  vec4 vColor;

void main()
{
    fragCoord = ciTexCoord0;
    vColor = ciColor;
    gl_Position	= ciModelViewProjection * ciPosition ;
}
