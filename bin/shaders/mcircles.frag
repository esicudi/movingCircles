#ifdef GL_ES
	#ifndef GL_FRAGMENT_PRECISION_HIGH
		#define highp mediump
	#endif
	precision highp float;
#endif

// inputs from vertex shader
in vec3 tc;

// output of the fragment shader
out vec4 fragColor;

// shader's global variables, called the uniform variables
uniform bool b_solid_color;
uniform vec4 solid_color;

void main()
{
	fragColor = b_solid_color ? vec4(tc,1) : solid_color;
}