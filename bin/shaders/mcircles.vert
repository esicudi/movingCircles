// input attributes of vertices
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;

// outputs of vertex shader = input to fragment shader
// out vec4 gl_Position: a built-in output variable that should be written in main()
out vec3 norm;
out vec3 tc;

// uniform variables
uniform mat4 model_matrix;
uniform mat4 aspect_matrix;

void main()
{
	gl_Position =  aspect_matrix*model_matrix*vec4(position,1);

	norm=vec3(0,0,-1.0f);
	tc=normal;
}