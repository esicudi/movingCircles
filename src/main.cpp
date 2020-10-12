#include "cgmath.h"
#include "cgut.h"
#include "circle.h"

//****************************************
// global constants
static const char*	window_name = "Moving Circles";
static const char*	vert_shader_path = "../bin/shaders/mcircles.vert";
static const char*	frag_shader_path = "../bin/shaders/mcircles.frag";
static const uint	TESS = 50;			// tessellation factor
static const uint	MIN_C = 20;			// minimum number of circles
static const uint	MAX_C = 512;		// maximum number of circles
uint				NUM_C = 32;			// initial number of circles

//****************************************
// window objects
GLFWwindow*	window = nullptr;
ivec2		window_size = ivec2(800, 450);

//****************************************
// OpenGL objects
GLuint	program = 0;
GLuint	vertex_array = 0;

//****************************************
// global variables
int		frame = 0;
float	t = 0.0f;
float	t0 = 0.0f;
bool	b_down = false;
bool	b_solid_color = false;
bool	b_index_buffer = true;
#ifndef GL_ES_VERSION_2_0
bool	b_wireframe = false;
#endif // !GL_ES_VERSION_2_0
auto	circles = std::move(create_circles(NUM_C));
struct { bool add = false, sub = false; operator bool() const { return add || sub; } } b;


//****************************************
// holder of vertices and indices of a unit circle
std::vector<vertex> unit_circle_vertices;

//****************************************
void update()
{
	// update global simulation parameter
	t = float(glfwGetTime()) - t0;
	t0 = float(glfwGetTime());

	// tricky aspect correction matrix for non-square window
	float aspect = window_size.x / float(window_size.y);
	//aspect *= 9.0f;
	//aspect /= 16.0f;
	mat4 aspect_matrix =
	{
		min(16.0f / (aspect*9.0f),1.0f), 0, 0, 0,
		0, min(aspect*9.0f/16.0f,1.0f), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// update common uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation(program, "b_solid_color");	if (uloc > -1) glUniform1i(uloc, b_solid_color);
	uloc = glGetUniformLocation(program, "aspect_matrix");	if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, aspect_matrix);

	// update vertex buffer by the pressed keys
	void update_c();
	if (b) update_c();
}

void render()
{
	// clear screen (with background color) and clear depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// notify GL that we use our own program
	glUseProgram(program);

	// bind vertex array object
	glBindVertexArray(vertex_array);

	// render N circles: trigger shader program to process vertex data
	for (auto& c : circles)
	{
		// per-circle update
		c.update(t);

		// update per-circle uniforms
		GLint uloc;
		uloc = glGetUniformLocation(program, "solid_color");	if (uloc > -1) glUniform4fv(uloc, 1, c.color);	// pointer version
		uloc = glGetUniformLocation(program, "model_matrix");	if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, c.model_matrix);

		// per-circle draw calls
		if (b_index_buffer)	glDrawElements(GL_TRIANGLES, TESS * 3, GL_UNSIGNED_INT, nullptr);
		else				glDrawArrays(GL_TRIANGLES, 0, TESS * 3);
	}

	// swap front and back buffers, and display to screen
	glfwSwapBuffers(window);
}

float collision_c(circle_t c1, circle_t c2)
{
	vec2 center1 = c1.center;
	vec2 center2 = c2.center;
	float a = c1.radius;
	float b = c2.radius;
	float d = sqrtf(powf((center1.x - center2.x)*16.0f/9.0f, 2) + powf(center1.y - center2.y, 2));
	float res;

	if ((a + b) < d) return -1.0f;

	float t1 = (powf(d, 2) + powf(a, 2) - powf(b, 2)) / (2 * a * d);
	float t2 = (powf(d, 2) - powf(a, 2) + powf(b, 2)) / (2 * b * d);

	res = (powf(a, 2) * (2 * acosf(t1) - sinf(2 * acosf(t1))) + powf(b, 2) * (2 * acosf(t2) - sinf(2 * acosf(t2)))) / 2;

	return res;
}

void col_elastic()
{
	int size = circles.size();
	for (int i = 0; i < size; i++)
	{
		circle_t& c1 = circles[i];

		for (int j = i + 1; j < size; j++)
		{
			circle_t& c2 = circles[j];
			float s0 = collision_c(c1, c2);
			if (s0 > 0.0f)
			{
				c1.center += (c1.velocity * 0.0000001f);
				c2.center += (c2.velocity * 0.0000001f);
				float s1 = collision_c(c1, c2);
				c1.center -= (c1.velocity * 0.0000001f);
				c2.center -= (c2.velocity * 0.0000001f);

				if (s1 >= s0)
				{
					vec2 v1, v2, v11, v22, n, v1y, v2y;
					n = c2.center - c1.center;

					n.x = n.x * 16.0f / 9.0f;
					n /= sqrt(pow(n.x, 2) + pow(n.y, 2));
					
					v1 = c1.velocity;
					v2 = c2.velocity;
					v1.x = v1.x * 16.0f / 9.0f;
					v2.x = v2.x * 16.0f / 9.0f;

					v1y = n * (n.x * v1.x + n.y * v1.y);
					v2y = n * (n.x * v2.x + n.y * v2.y);

					v11 = ((pow(c1.radius, 2) - pow(c2.radius, 2)) * v1y + 2 * pow(c2.radius, 2) * v2y) / (pow(c1.radius, 2) + pow(c2.radius, 2));
					v22 = ((pow(c2.radius, 2) - pow(c1.radius, 2)) * v2y + 2 * pow(c1.radius, 2) * v1y) / (pow(c1.radius, 2) + pow(c2.radius, 2));

					c1.velocity = (v1 - v1y + v11);
					c1.velocity.x *= (9.0f / 16.0f);
					c2.velocity = (v2 - v2y + v22);
					c2.velocity.x *= (9.0f / 16.0f);
				}
			}
		}

		vec2 pos = c1.center;
		float radius = c1.radius;
		if (pos.x - (radius * 9.0f / 16.0f) < -1.0f)
		{
			if (c1.velocity.x < 0.0) c1.velocity.x *= -1.0;
		}
		else if (pos.x + (radius * 9.0f / 16.0f) > 1.0f)
		{
			if (c1.velocity.x > 0.0) c1.velocity.x *= -1.0;
		}
		else if ((pos.y - radius) < -1.0f)
		{
			if (c1.velocity.y < 0.0) c1.velocity.y *= -1.0;
		}
		else if ((pos.y + radius) > 1.0f)
		{
			if (c1.velocity.y > 0.0) c1.velocity.y *= -1.0;
		}
	}
}

void reshape(GLFWwindow* window, int width, int height)
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width, height);
	glViewport(0, 0, width, height);
}

void print_help()
{
	printf("[help]\n");
	printf("- press ESC or 'q' to terminate the program\n");
	printf("- press F1 or 'h' to see help\n");
	printf("- press 'd' to toggle between solid color and texture coordinates\n");
	printf("- press '+/-' to increase/decrease the number of circle (min=20, max=512)\n");
	printf("- press 'i' to toggle between index buffering and simple vertex buffering\n");
#ifndef GL_ES_VERSION_2_0
	printf("- press 'w' to toggle wireframe\n");
#endif
	printf("- press 'r' to reset circles\n");
	printf("\n");
}

std::vector<vertex> create_circle_vertices(uint N)
{
	std::vector<vertex> v = { {vec3(0),vec3(1.0f,0.5f,0.5f),vec2(0.0f)} };
	for (uint k = 0; k <= N; k++)
	{
		float t = PI * 2.0f * k / float(N), c = cos(t) * 9.0f / 16.0f, s = sin(t);
		//v.push_back({ vec3(c,s,0),vec3(0,0,-1.0f),vec2(c,s) * 0.5f = 0.5f });
		//v.push_back({ vec3(c,s,0), vec3(0,0,-1.0f), vec2(c,s) * 0.5f + 0.5f });

		v.push_back({ vec3(c,s,0),vec3(1.0f,0.5f,0.5f),vec2(0.0f) });

		//c = { vec2(-0.5f,0),1.0f,0.0f,vec4(1.0f,0.5f,0.5f,1.0f) };
	}
	return v;
}

void update_vertex_buffer(const std::vector<vertex>& vertices, uint N)
{
	static GLuint vertex_buffer = 0;
	static GLuint index_buffer = 0;

	// check exceptions
	if (vertices.empty()) { printf("[error] vertices is empty.\n"); return; }

	// create buffers
	if (b_index_buffer)
	{
		std::vector<uint> indices;
		for (uint k = 0; k < N; k++)
		{
			indices.push_back(0);
			indices.push_back(k + 1);
			indices.push_back(k + 2);
		}

		// generation of vertex buffer: use vertices as it is
		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

		// geneation of index buffer
		glGenBuffers(1, &index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * indices.size(), &indices[0], GL_STATIC_DRAW);
	}
	else
	{
		std::vector<vertex> v;
		for (uint k = 0; k < N; k++)
		{
			v.push_back(vertices.front());
			v.push_back(vertices[k + 1]);
			v.push_back(vertices[k + 2]);
		}

		// generation of vertex buffer: use triangle_vertices instead of vertices

		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * v.size(), &v[0], GL_STATIC_DRAW);
	}

	// generate vertex array object, which is mandatory for OpenGL 3.3 and higher
	if (vertex_array) glDeleteVertexArrays(1, &vertex_array);
	vertex_array = cg_create_vertex_array(vertex_buffer, index_buffer);
	if (!vertex_array) { printf("%s(): failed to create vertex array\n", __func__); return; }
}

void window_reset()
{
	circles = std::move(create_circles(NUM_C));
	unit_circle_vertices = create_circle_vertices(TESS);
	update_vertex_buffer(unit_circle_vertices, TESS);
}

void update_c()
{
	uint n = NUM_C; if (b.add) { n++; b.add = false; } if (b.sub) { n--; b.sub = false; }
	if (n == NUM_C || n<MIN_C || n>MAX_C) return;

	NUM_C = n;
	window_reset();

	printf("> number of circles = % -4d\r", NUM_C);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)	glfwSetWindowShouldClose(window, GL_TRUE);
		else if (key == GLFW_KEY_F1 || key == GLFW_KEY_H)	print_help();
		else if (key == GLFW_KEY_KP_ADD || (key == GLFW_KEY_EQUAL && (mods & GLFW_MOD_SHIFT))) b.add = true;
		else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) b.sub = true;
		else if (key == GLFW_KEY_I)
		{
			b_index_buffer = !b_index_buffer;
			update_vertex_buffer(unit_circle_vertices, TESS);
			printf("> using %s buffering   \n", b_index_buffer ? "index" : "vertex");
		}
		else if (key == GLFW_KEY_D)
		{
			b_solid_color = !b_solid_color;
			printf("> using %s       \n", b_solid_color ? "solid color" : "texture coordinates as color");
		}
		else if (key == GLFW_KEY_R)
		{
			window_reset();
			printf("> reset circles           \n");
		}
#ifndef GL_ES_VERSION_2_0
		else if (key == GLFW_KEY_W)
		{
			b_wireframe = !b_wireframe;
			glPolygonMode(GL_FRONT_AND_BACK, b_wireframe ? GL_LINE : GL_FILL);
			printf("> using %s mode        \n", b_wireframe ? "wireframe" : "solid");
		}
#endif
	}
	else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_KP_ADD || (key == GLFW_KEY_EQUAL && (mods & GLFW_MOD_SHIFT))) b.add = false;
		else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) b.sub = false;
	}
}

void mouse(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			b_down = true;
		}
		else if (action == GLFW_RELEASE)
		{
			b_down = false;
			dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
			printf("> Left mouse button pressed at (%3d, %3d)\n", int(pos.x), int(pos.y));
		}
	}
}

void motion(GLFWwindow* window, double x, double y)
{
	if (b_down == true)
	{
		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
		printf("> Left mouse button pressed at (%3d, %3d)\r", int(pos.x), int(pos.y));
	}
}

bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glLineWidth(1.0f);
	glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// define the position of four courner vertices
	unit_circle_vertices = std::move(create_circle_vertices(TESS));

	// create vertex buffer; called again when index buffering mode is toggled
	update_vertex_buffer(unit_circle_vertices, TESS);
	
	return true;
}

void user_finalize()
{

}

/*
추가기능:
> 속도 빠르게/느리게
> 일시정지
> 원 클릭-드래그
*/

int main(int argc, char* argv[])
{
	// create window and initialize OpenGL extensions
	if (!(window = cg_create_window(window_name, window_size.x, window_size.y))) { glfwTerminate(); return 1; }
	if (!cg_init_extensions(window)) { glfwTerminate(); return 1; }

	// initializations and validations of GLSL program
	if (!(program = cg_create_program(vert_shader_path, frag_shader_path))) { glfwTerminate(); return 1; }
	if (!user_init()) { printf("Failed to user_init()\n"); glfwTerminate(); return 1; }

	// register event callbacks
	glfwSetWindowSizeCallback(window, reshape);
	glfwSetKeyCallback(window, keyboard);
	glfwSetMouseButtonCallback(window, mouse);
	glfwSetCursorPosCallback(window, motion);

	t0 = float(glfwGetTime());
	// enters rendering/event loop
	for (frame = 0; !glfwWindowShouldClose(window); frame++)
	{
		glfwPollEvents();
		update();
		col_elastic();
		render();
	}

	// normal termination
	user_finalize();
	cg_destroy_window(window);

	return 0;
}