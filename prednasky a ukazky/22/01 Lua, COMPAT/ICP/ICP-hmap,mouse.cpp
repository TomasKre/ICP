// icp.cpp 
// Author: JJ

// C++ 
#include <iostream>
#include <chrono>

// OpenCV 
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler
#include <GL/glew.h> 
#include <GL/wglew.h> //WGLEW = Windows GL Extension Wrangler (change for different platform) 

// GLFW toolkit
#include <GLFW/glfw3.h>

// OpenGL math
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// OpenGL textures
#include <gli/gli.hpp>

//project includes...
#include "globals.h"
#include "glerror.h" // Check for GL errors

#include "mesh.h"
#include "mesh_init.h"

//mesh
mesh mesh_circle;
int circle_segments = 1'000'000; 
mesh mesh_cube;
mesh height_map;
unsigned int draw_method;


typedef struct  Avatar {                            // player (viewer) info
	float       posX;
	float       posY; //height
	float       posZ;
	float       angle;
} Avatar;

Avatar  avatar = { 5000.0, 0.0, 5000.0, 0.0 };
Avatar  AI[];
std::vector<Avatar> bullets;
Avatar* camera;

//=====================================================================================================
//FORWARD DECLARATIONS
static void error_callback(int error, const char* description);
static void init_glfw(void);
static void init_opengl(void);
static void init_glew(void);
static void gl_print_info(void);
static void finalize(int code);

static void fbsize_callback(GLFWwindow* window, int width, int height);
//=====================================================================================================
static mesh HeightMap(const cv::Mat& hmap, const unsigned int mesh_step_size)
{
	mesh mesh;
	glm::vec3 vertex;
	glm::vec4 color;

	if (hmap.empty())
		return mesh;

	std::cout << "Note: heightmap size:" << hmap.size << ", channels: " << hmap.channels() << std::endl;

	if (hmap.channels() != 1)
	{
		std::cerr << "WARN: requested 1 channel, got: " << hmap.channels() << std::endl;
	}

	// Create heightmap mesh from QUADS in XZ plane, Y is UP (right hand rule) 
	for (unsigned int x_coord = 0; x_coord < (hmap.cols - mesh_step_size); x_coord += mesh_step_size)
	{
		for (unsigned int z_coord = 0; z_coord < (hmap.rows - mesh_step_size); z_coord += mesh_step_size)
		{
			// Get The (X, Y, Z) Value For The Bottom Left Vertex
			vertex.x = x_coord;
			vertex.y = hmap.at<uchar>(cv::Point(x_coord, z_coord));
			vertex.z = z_coord;
			// Set The Color Value Of The Current Vertex (grayscale image returns 0..256, opengl color is 0.0f..1.0f) 
			color.r = color.g = color.b = hmap.at<uchar>(cv::Point(x_coord, z_coord)) / 256.0f;
			color.a = 1.0f;
			// Store vertex & color
			mesh.vertices.push_back(vertex);
			mesh.colors.push_back(color);

			// Get The (X, Y, Z) Value For The Top Left Vertex
			vertex.x = x_coord;
			vertex.y = hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size));
			vertex.z = z_coord + mesh_step_size;
			// Set The Color Value Of The Current Vertex
			color.r = color.g = color.b = hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size)) / 256.0f;
			color.a = 1.0f;
			// Store vertex & color
			mesh.vertices.push_back(vertex);
			mesh.colors.push_back(color);

			// Get The (X, Y, Z) Value For The Top Right Vertex
			vertex.x = x_coord + mesh_step_size;
			vertex.y = hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size));
			vertex.z = z_coord + mesh_step_size;
			// Set The Color Value Of The Current Vertex
			color.r = color.g = color.b = hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size)) / 256.0f;
			color.a = 1.0f;
			// Store vertex & color
			mesh.vertices.push_back(vertex);
			mesh.colors.push_back(color);

			// Get The (X, Y, Z) Value For The Bottom Right Vertex
			vertex.x = x_coord + mesh_step_size;
			vertex.y = hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord));
			vertex.z = z_coord;
			// Set The Color Value Of The Current Vertex
			color.r = color.g = color.b = hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord)) / 256.0f;
			color.a = 1.0f;
			// Store vertex & color
			mesh.vertices.push_back(vertex);
			mesh.colors.push_back(color);
		}
	}

	mesh.primitive_type = GL_QUADS;
	mesh.colors_used = true;

	return mesh;
}

void DrawScene(void)
{
	//
	// DRAW
	//

	// plane
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_QUADS);
	glVertex3f(5000, 0, 5000);
	glVertex3f(5000, 0, -5000);
	glVertex3f(-5000, 0, -5000);
	glVertex3f(-5000, 0, 5000);
	glEnd();


	// circle and cube have size cca 1.0f, need to rescale
	glScalef(1000.0f, 1000.0f, 1000.0f);

	// draw green cube
	glTranslatef(0.0f, 2.0f, 0.0f);
	glColor3f(0.5f, 1.0f, 0.5f);
	mesh_draw_arrays(mesh_cube);
	glTranslatef(0.0f, -2.0f, 0.0f);

	// draw circle using different methods
	glColor3f(1.0f, 1.0f, 1.0f);
	switch (draw_method)
	{
	case 0:
		mesh_draw_vertex(mesh_circle);
		break;
	case 1:
		mesh_draw_arrays(mesh_circle);
		break;
	case 2:
		mesh_draw_arrayelement(mesh_circle);
		break;
	case 3:
		mesh_draw_elements(mesh_circle);
		break;
	case 9:
	{
		int i;
		float theta, x, y;
		const float r = 1.0; //radius

		glBegin(GL_LINE_LOOP);
		for (i = 0; i < circle_segments; i++)
		{
			theta = 2.0f * 3.1415926f * float(i) / float(circle_segments);

			x = r * cosf(theta);
			y = r * sinf(theta);

			glVertex2f(x, y);
		}
		glEnd();
	}
	default:
		break;
	}
}

static void app_loop(void)
{
	// Time measurement, FPS count etc.
	static double time_fps_old = 0.0;
	static double time_frame_old = 0.0;
	static int frame_cnt = 0;
	double time_current, time_frame_delta;

	// Run until exit is requested.
	while (!glfwWindowShouldClose(globals.window))
	{
		time_current = glfwGetTime();
		time_frame_delta = time_current - time_frame_old;
		time_frame_old = time_current;

		//FPS
		if (time_current - time_fps_old > 1.0)
		{
			time_fps_old = time_current;
			std::cout << "FPS: " << frame_cnt << std::endl;
			frame_cnt = 0;
		}
		frame_cnt++;

		// Clear color buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Use ModelView matrix for following trasformations (translate,rotate,scale)
		glMatrixMode(GL_MODELVIEW);

		// Set the camera (eye, center, up)
		glm::mat4 v_m;
//		glm::mat4 v_m = glm::lookAt(glm::vec3(5000.0f, 0.0f, 5000.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		v_m = glm::lookAt(glm::vec3(avatar.posX, avatar.posY, avatar.posZ), glm::vec3(avatar.posX + cosf(avatar.angle), avatar.posY, avatar.posZ + sinf(avatar.angle)), glm::vec3(0.0f, 1.0f, 0.0f));
		
		glLoadMatrixf(glm::value_ptr(v_m));

		//
		// Draw something using OpenGL - no shaders yet ;-) 
		//

		// Scale Value For The Terrain - sizeo of terrain is from raster = 1024x1024
		// scale (or move further from camera...)
		float scaleValue = 3.0f;
		glScalef(scaleValue, scaleValue, scaleValue);
		mesh_draw(height_map);
		glScalef(1.0 / scaleValue, 1.0 / scaleValue, 1.0 / scaleValue);

		// Render here 
		// ...
		DrawScene();
		// ...

		// Swap front and back buffers 
		// Calls glFlush() inside
		glfwSwapBuffers(globals.window);

		// Check OpenGL errors
		gl_check_error();

		// Poll for and process events
		glfwPollEvents();
	}
}

static void init_mesh(void)
{
	mesh_circle = gen_mesh_circle(1.0f, circle_segments);
	std::cout << "Mesh: CIRCLE initialized, vertices: " << mesh_circle.vertices.size() << ", indices: " << mesh_circle.indices.size() << std::endl;


	if (!loadOBJ(mesh_cube, "cube_triangles_normals_tex.obj"))
	{
		std::cerr << "loadOBJ failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::cout << "Mesh: CUBE initialized, vertices: " << mesh_cube.vertices.size() << ", indices: " << mesh_cube.indices.size() << std::endl;

	// height map
	{
		std::string hm_file("heights.png");
		cv::Mat hmap = cv::imread(hm_file, cv::IMREAD_GRAYSCALE);

		if (hmap.empty())
		{
			std::cerr << "ERR: Height map empty? File:" << hm_file << std::endl;
		}

		height_map = HeightMap(hmap, 10); //image, step size
		std::cout << "Note: height map vertices: " << height_map.vertices.size() << std::endl;
	}

}

static void init(void)
{
	init_opengl();
	init_mesh();
}

int main(int argc, char* argv[])
{
	init();

	app_loop();

	finalize(EXIT_SUCCESS);
}

//############################################################################################################################
static void init_opengl(void)
{
	init_glfw();
	init_glew();
	gl_print_info();
}

static void error_callback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	static GLfloat point_size = 1.0f;

	if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
	{
		switch (key) {
		case GLFW_KEY_ESCAPE: //fallthrough
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case GLFW_KEY_0:
			draw_method = 0;
			std::cout << "Draw method: vertex for-loop" << std::endl;
			break;
		case GLFW_KEY_1:
			draw_method = 1;
			std::cout << "Draw method: arrays" << std::endl;
			break;
		case GLFW_KEY_2:
			draw_method = 2;
			std::cout << "Draw method: indirect for-loop" << std::endl;
			break;
		case GLFW_KEY_3:
			draw_method = 3;
			std::cout << "Draw method: indirect arrays" << std::endl;
			break;
		case GLFW_KEY_9:
			draw_method = 9;
			std::cout << "Draw method: computational" << std::endl;
			break;
		case GLFW_KEY_S:
			avatar.posZ += 25;
			break;
		case GLFW_KEY_W:
			avatar.posZ -= 25;
			break;
		case GLFW_KEY_A:
			avatar.posX -= 25;
			break;
		case GLFW_KEY_D:
			avatar.posX += 25;
			break;
		default:
			break;
		}
	}
}

//---------------------------------------------------------------------
// Mouse pressed?
//---------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			//action
			std::cout << "BTNL" << std::endl;
		}
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			std::cout << "BTNR" << std::endl;
			//action
		}
	}
}

//---------------------------------------------------------------------
// Mose moved?
//---------------------------------------------------------------------
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	static int first = 1;
	static int old_x;
	if (first) {
		old_x = xpos;
		first = 0;
	}
	else {
		avatar.angle = (-xpos + old_x)/180.0;
	}
	std::cout << "move: " << xpos << "," << ypos << std::endl;
}

static void fbsize_callback(GLFWwindow* window, int width, int height)
{
	// check for limit case (prevent division by 0)
	if (height == 0)
		height = 1;

	float ratio = width * 1.0f / height;

	globals.width = width;
	globals.height = height;

	glMatrixMode(GL_PROJECTION);				// set projection matrix for following transformations

	glm::mat4 projectionMatrix = glm::perspective(
		glm::radians(45.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
		ratio,			     // Aspect Ratio. Depends on the size of your window.
		0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
		20000.0f              // Far clipping plane. Keep as little as possible.
	);
	glLoadMatrixf(glm::value_ptr(projectionMatrix));

	glViewport(0, 0, width, height);			// set visible area

	std::cout << width << " " << height << std::endl;
}
//############################################################################################################################

static void init_glfw(void)
{
	//
	// GLFW init.
	//

	// set error callback first
	glfwSetErrorCallback(error_callback);

	//initialize GLFW library
	int glfw_ret = glfwInit();
	if (!glfw_ret)
	{
		std::cerr << "GLFW init failed." << std::endl;
		finalize(EXIT_FAILURE);
	}

	// Shader based, modern OpenGL (3.3 and higher)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // only new functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // only old functions (for old tutorials etc.)

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	globals.window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
	if (!globals.window)
	{
		std::cerr << "GLFW window creation error." << std::endl;
		finalize(EXIT_FAILURE);
	}

	// Get some GLFW info.
	{
		int major, minor, revision;

		glfwGetVersion(&major, &minor, &revision);
		std::cout << "Running GLFW " << major << '.' << minor << '.' << revision << std::endl;
		std::cout << "Compiled against GLFW " << GLFW_VERSION_MAJOR << '.' << GLFW_VERSION_MINOR << '.' << GLFW_VERSION_REVISION << std::endl;
	}

	glfwMakeContextCurrent(globals.window);										// Set current window.
	glfwGetFramebufferSize(globals.window, &globals.width, &globals.height);	// Get window size.
	glfwSwapInterval(0);														// Set V-Sync OFF.
	//glfwSwapInterval(1);														// Set V-Sync ON.

	//register callbacks
	glfwSetKeyCallback(globals.window, key_callback);
	glfwSetFramebufferSizeCallback(globals.window, fbsize_callback);			// On window resize callback.
	glfwSetMouseButtonCallback(globals.window, mouse_button_callback);
	glfwSetCursorPosCallback(globals.window, cursor_position_callback);
	fbsize_callback(globals.window, globals.width, globals.height);				//initial GL window size setting - call callback manually

	globals.app_start_time = glfwGetTime();										// Get start time.

}

static void init_glew(void)
{
	//
	// Initialize all valid GL extensions with GLEW.
	// Usable AFTER creating GL context!
	//
	{
		GLenum glew_ret;
		glew_ret = glewInit();
		if (glew_ret != GLEW_OK)
		{
			std::cerr << "WGLEW failed with error: " << glewGetErrorString(glew_ret) << std::endl;
			finalize(EXIT_FAILURE);
		}
		else
		{
			std::cout << "GLEW successfully initialized to version: " << glewGetString(GLEW_VERSION) << std::endl;
		}

		// Platform specific. (Change to GLXEW or ELGEW if necessary.)
		glew_ret = wglewInit();
		if (glew_ret != GLEW_OK)
		{
			std::cerr << "WGLEW failed with error: " << glewGetErrorString(glew_ret) << std::endl;
			finalize(EXIT_FAILURE);
		}
		else
		{
			std::cout << "WGLEW successfully initialized platform specific functions." << std::endl;
		}
	}
}

static void gl_print_info(void)
{
	// Get OpenGL driver info
	{
		const char* vendor_s = (const char*)glGetString(GL_VENDOR);
		const char* renderer_s = (const char*)glGetString(GL_RENDERER);
		const char* version_s = (const char*)glGetString(GL_VERSION);
		const char* glsl_s = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

		std::cout << "OpenGL driver vendor: " << (vendor_s == nullptr ? "<UNKNOWN>" : vendor_s) << ", renderer: " << (renderer_s == nullptr ? "<UNKNOWN>" : renderer_s) << ", version: " << (version_s == nullptr ? "<UNKNOWN>" : version_s) << std::endl;

		GLint profile_flags;
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_flags);
		std::cout << "Current profile: ";
		if (profile_flags & GL_CONTEXT_CORE_PROFILE_BIT)
			std::cout << "CORE";
		else
			std::cout << "COMPATIBILITY";
		std::cout << std::endl;

		GLint context_flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
		std::cout << "Active context flags: ";
		if (context_flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
			std::cout << "GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT ";
		if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)
			std::cout << "GL_CONTEXT_FLAG_DEBUG_BIT ";
		if (context_flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)
			std::cout << "GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT ";
		if (context_flags & GL_CONTEXT_FLAG_NO_ERROR_BIT)
			std::cout << "GL_CONTEXT_FLAG_NO_ERROR_BIT";
		std::cout << std::endl;

		std::cout << "Primary GLSL shading language version: " << (glsl_s == nullptr ? "<UNKNOWN>" : glsl_s) << std::endl;
	}

	//
	// GLM & GLI library
	//

	std::cout << "GLM version: " << GLM_VERSION_MAJOR << '.' << GLM_VERSION_MINOR << '.' << GLM_VERSION_PATCH << "rev" << GLM_VERSION_REVISION << std::endl;
	std::cout << "GLI version: " << GLI_VERSION_MAJOR << '.' << GLI_VERSION_MINOR << '.' << GLI_VERSION_PATCH << "rev" << GLI_VERSION_REVISION << std::endl;
}

static void finalize(int code)
{
	cv::destroyAllWindows();

	// Close OpenCV capture device
	if (globals.capture.isOpened())
		globals.capture.release();

	// Close OpenGL window if opened and terminate GLFW  
	if (globals.window)
		glfwDestroyWindow(globals.window);
	glfwTerminate();

	exit(code);
}
