#version 330

// input from vertex shader
in vec2 tc;

// the only output variable
out vec4 fragColor;

// uniform variables
uniform sampler2D TEX;
uniform bool snap;

void main()
{
	if(snap == false){fragColor =  texture2D(TEX, tc);}
	else{fragColor = vec4(1.f ,1.f ,1.f ,1.f);}
}