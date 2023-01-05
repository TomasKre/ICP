// icp.cpp 
// Author: JJ

// C++ 
#include <iostream>
#include <chrono>
#include <stack>
#include <random>

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
#include "init.h"
#include "callbacks.h"
#include "glerror.h" // Check for GL errors

#include "mesh.h"
#include "mesh_init.h"
#include "texture.h"
#include "lua_engine.h"
#include "lua_interface.h"

#include "shaders.h"

//mesh
mesh mesh_floor;
mesh mesh_brick;
mesh mesh_treasure;
mesh mesh_gun;
std::vector<mesh> meshes_magazine;

std::stack<glm::mat4> stack_modelview;

Avatar  player = { 10.0f, 0.0f, 0.0f, 0.0f };
Avatar  shot = { 0.0, 0.0, 0.0 };

int bullet = 5;							//bullet count 
bool fireBullet = false;				//is fired?
bool enableLight = true;
bool bulletTime = false;

constexpr auto BLOCK_SIZE = 10.0f;

cv::Mat mapa = cv::Mat(10,25, CV_8U);   // maze map

//shaders
GLuint shader_id;
bool use_shader = false;

// secure access to map
uchar getmap(cv::Mat& map, int x, int y)
{
	if (x < 0)
	{
		std::cerr << "Map: X too small: " << x << std::endl;
		x = 0;
	}

	if (x >= map.cols)
	{
		std::cerr << "Map: X too big: " << x << std::endl;
		x = map.cols - 1;
	}

	if (y < 0)
	{
		std::cerr << "Map: Y too small: " << y << std::endl;
		y = 0;
	}

	if (y >= map.rows)
	{
		std::cerr << "Map: Y too big: " << y << std::endl;
		y = map.rows - 1;
	}

	//at(row,col)!!!
	return map.at<uchar>(y,x);
}

// forward declarations
static void local_init(void);
static void local_init_mesh(void);
static unsigned char collision(Avatar* avatar);
//---------------------------------------------------------------------
// Random map gen
//---------------------------------------------------------------------
void genLabyrinth(void) {
	int i, j;
	cv::Point2i start, end;

	// C++ random numbers
	std::random_device r; // Seed with a real random value, if available
	std::default_random_engine e1(r());
	std::uniform_int_distribution<int> uniform_height(1, mapa.rows - 2); // uniform distribution between int..int
	std::uniform_int_distribution<int> uniform_width(1, mapa.cols - 2);
	std::uniform_int_distribution<int> uniform_block(0, 15); // jak ridce jsou generovane zdi: 0=zed, vse ostatni=volno

	//inner maze 
	for (j = 0; j < mapa.rows; j++) {
		for (i = 0; i < mapa.cols; i++) {

			switch (uniform_block(e1))
			{
			case 0:
				mapa.at<uchar>(cv::Point(i, j)) = '#';
				break;
			default:
				mapa.at<uchar>(cv::Point(i, j)) = '.';
				break;
			}
		}
	}

	//walls
	for (i = 0; i < mapa.cols; i++) {
		mapa.at<uchar>(cv::Point(i, 0)) = '#';
		mapa.at<uchar>(cv::Point(i, mapa.rows - 1)) = '#';
	}
	for (j = 0; j < mapa.rows; j++) {
		mapa.at<uchar>(cv::Point(0, j)) = '#';
		mapa.at<uchar>(cv::Point(mapa.cols - 1, j)) = '#';
	}

	//gen start inside maze (excluding walls)
	do {
		start.x = uniform_width(e1);
		start.y = uniform_height(e1);
	} while (getmap(mapa, start.x, start.y) == '#'); //check wall

	//gen end different from start, inside maze (excluding outer walls) 
	do {
		end.x = uniform_width(e1);
		end.y = uniform_height(e1);
	} while (start == end); //check overlap
	mapa.at<uchar>(cv::Point(end.x, end.y)) = 'e';

	std::cout << "Start: " << start << std::endl;
	std::cout << "End: " << end << std::endl;

	//print map
	for (j = 0; j < mapa.rows; j++) {
		for (i = 0; i < mapa.cols; i++) {
			if ((i == start.x) && (j == start.y))
				std::cout << 'X';
			else
				std::cout << getmap(mapa, i, j);
		}
		std::cout << std::endl;
	}

	//set player position
	player.posX = (-start.x * BLOCK_SIZE) - BLOCK_SIZE / 2.0f;
	player.posY = (-start.y * BLOCK_SIZE) - BLOCK_SIZE / 2.0f;
}

//---------------------------------------------------------------------
// position & draw bullet 
//---------------------------------------------------------------------
void shotMoveView(Avatar* shot)
{
	glPushMatrix();

	glRotatef(-90, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0, 4.9, 0.0);
	glTranslatef(-shot->posX, 0.0, -shot->posY);
	glRotatef(shot->move_h_angle, 0.0f, 1.0f, 0.0f);
	
	glScalef(0.3f,0.3f,0.3f);
	mesh_draw(mesh_gun);     //strelba zmensenou pistoli :-D (to nikdo nevidi, tak co...)

	glPopMatrix();
}

//---------------------------------------------------------------------
// Avatar view and orientation
//---------------------------------------------------------------------
void avatarMoveView(Avatar* avatar)
{
	constexpr auto MOVE_SCENE_DOWN = -5.0;            // make x-y plane visible;
	constexpr auto SCENE_SHIFT = -1.0;                // move scene for rotation;

	glTranslatef(0.0, MOVE_SCENE_DOWN, 0.0);        // move down - make z-plane visible
	glRotatef(90.0, 1.0, 0.0, 0.0);                 // change axis
	glTranslatef(0.0, SCENE_SHIFT, 0.0);            // step back for rotation
	glRotatef(avatar->cam_h_angle, 0.0, 0.0, 1.0);      // rotate player
	glTranslatef(avatar->posX, avatar->posY, 0.0);  // move player
}

//---------------------------------------------------------------------
// Collision detection with walls
//---------------------------------------------------------------------	
static unsigned char collision(Avatar* avatar)
{
	int kvadrantX, kvadrantY;

	//avatar quadrant
	kvadrantX = ceilf(avatar->posX / BLOCK_SIZE);
	kvadrantY = ceilf(avatar->posY / BLOCK_SIZE);

	//TODO: check together with avatar (shot, player) size !!!
	
	//TODO: probably new function:
	//		check X,Y separately, allow movement in non-blocked direction
	//		allow largest possible step in blocked direction
	//		return & apply largest allowed step	

	return getmap(mapa, abs(kvadrantX), abs(kvadrantY));
}

//---------------------------------------------------------------------
// 3D draw
//---------------------------------------------------------------------
void DrawAll(void)
{
	int i, j;
	static double old_time = 0.0;
	static double old_frame_time = 0.0;
	static int frame_cnt = 0;
	double current_time;

	current_time = glfwGetTime();
	
	// move bullet each 10ms if active
	if ( (fireBullet == true) && (current_time - old_time > 0.01) )
	{
		old_time = current_time;

		Avatar a = avatarMoveForward(shot);
		switch (collision(&a))
		{
		case '#':
			fireBullet = false;
			break;
		case 'e':
			fireBullet = false;
			genLabyrinth();
			return;
			break;
		case '.': //fallthrough
		default:
			shot = a;
			break;
		}
		
		if (!fireBullet)
		{
			globals.camera = &player;
			bulletTime = false;
		}
	}
	

	//write FPS
	if (current_time - old_frame_time > 1.0)
	{
		old_frame_time = current_time;
		std::cout << "FPS: " << frame_cnt << "\r"; 
		frame_cnt = 0;
	}
	frame_cnt++;

	//light on/off
	if (enableLight == true)
	{
		glEnable(GL_LIGHT0);
	}
	else {
		glDisable(GL_LIGHT0);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	avatarMoveView(globals.camera);

	//bullet fired
	if (fireBullet == true) {
		shotMoveView(&shot);
	}

	//draw gun
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -3.0);
	if (!bulletTime) mesh_draw(mesh_gun);
	mesh_draw(meshes_magazine[bullet]);
	glPopMatrix();

	//draw ground
	mesh_draw(mesh_floor);

	// draw inner walls and treasure
	for (j = 0; j < mapa.rows; j++)
	{
		for (i = 0; i < mapa.cols; i++)
		{
			switch (getmap(mapa, i, j))
			{
			case '.':
				continue;
				break;
			case 'X':
				continue;
				break;
			case '#':
				glPushMatrix();
				glTranslatef(i * BLOCK_SIZE, j * BLOCK_SIZE, 0.0f);
				mesh_draw(mesh_brick);
				glPopMatrix();
				break;
			case 'e':
				glPushMatrix();
				glTranslatef(i * BLOCK_SIZE, j * BLOCK_SIZE, 0.0f);
				mesh_draw(mesh_treasure);
				glPopMatrix();
				break;
			default:
				break;
			}
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
			if (!fireBullet && bullet != 0) {
				bullet = bullet - 1;
				fireBullet = true;
				shot.move_h_angle = shot.cam_h_angle = player.move_h_angle;
				shot.posX = player.posX;
				shot.posY = player.posY;
			}
		}
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			if (bullet != 5) bullet = 5;
		}
	}
}

//---------------------------------------------------------------------
// Mouse moved?
//---------------------------------------------------------------------
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	static int first = 1;
	static double old_x;
	if (first) {
		old_x = xpos;
		first = 0;
	}
	else {
		*globals.camera = avatarRotate(*globals.camera, glm::radians(-xpos + old_x), glm::radians(0.0f), glm::radians(0.0f));
	}
}

//---------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------
int main(int argc, char** argv)
{
	// Call all initialization.
	init_glfw();
	init_glew();
	gl_print_info();

	local_init();
	local_init_mesh();

	// Run until exit is requested.
	while (!glfwWindowShouldClose(globals.window))
	{
		// Clear color buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Use ModelView matrix for following trasformations (translate,rotate,scale)
		glMatrixMode(GL_MODELVIEW);
		// Clear all tranformations
		glLoadIdentity();

		lua_pre_run();

		if (globals.lua.run() != EXIT_SUCCESS)
		{
			std::cerr << "[CPP," << glfwGetTime() << "] Lua failed to run." << std::endl;
			finalize(EXIT_FAILURE);
		}

		lua_post_run();

		DrawAll();

		// Swap front and back buffers 
		// Calls glFlush() inside
		glfwSwapBuffers(globals.window);

		// Check for errors in current frame
		gl_check_error();

		// Poll for and process events
		glfwPollEvents();
	}

	finalize(EXIT_SUCCESS);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Avatar a = player;

	if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE: //fallthrough
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_UP: //fallthrough
		case GLFW_KEY_W:
			a = avatarMoveForward(a);
			break;
		case GLFW_KEY_DOWN: //fallthrough
		case GLFW_KEY_S:
			a = avatarMoveBackward(a);
			break;
		case GLFW_KEY_LEFT: //fallthrough
		case GLFW_KEY_A:
			a = avatarMoveLeft(a);
			break;
		case GLFW_KEY_RIGHT: //fallthrough
		case GLFW_KEY_D:
			a = avatarMoveRight(a);
			break;
		case GLFW_KEY_PAGE_UP: //fallthrough
		case GLFW_KEY_R:
			a = avatarMoveUp(a);
			break;
		case GLFW_KEY_PAGE_DOWN: //fallthrough
		case GLFW_KEY_F:
			a = avatarMoveDown(a);;
			break;
		case GLFW_KEY_KP_ADD:
			a.movement_speed += 1.0f;
			std::cout << "Speed: " << a.movement_speed << std::endl;
			break;
		case GLFW_KEY_KP_SUBTRACT:
			if (a.movement_speed > 1.0f)
				a.movement_speed -= 1.0f;
			std::cout << "Speed: " << a.movement_speed << std::endl;
			break;
		case GLFW_KEY_L:
			enableLight = !enableLight;
			std::cout << "Light: " << enableLight << std::endl;
			break;
		case GLFW_KEY_B:
			if (fireBullet)	bulletTime = !bulletTime;
			if (bulletTime)
				globals.camera = &shot;
			else
				globals.camera = &player;
			break;
		case GLFW_KEY_I: // get value from Lua VM
			std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<int>("i") << "' for variable 'i'." << std::endl;
			break;
		case GLFW_KEY_M: // force Lua to print its current map
				//call function in Lua script
				lua_getglobal(globals.lua.L, "hello");
				if (lua_isfunction(globals.lua.L, -1))
				{
					lua_pushstring(globals.lua.L, "GLFW_KEY_M => PRINT MAP");
					lua_pcall(globals.lua.L, 1, 0, 0);
				}
			break;
		case GLFW_KEY_U:
			if (use_shader)
			{
				glUseProgram(0); // if common for all, Use just once
			}
			else
			{
				glUseProgram(shader_id); // if common for all, Use just once
			}
			use_shader = !use_shader;
			break;
		default:
			break;
		}

		if (collision(&a) == '.') {
			player = a;
		}
	}
}

static void local_init_mesh(void)
{
	genLabyrinth();	// create labyrinth
	
	mesh_floor = gen_mesh_floor(mapa, BLOCK_SIZE);
	mesh_gun = gen_mesh_gun();
	mesh_brick = gen_mesh_brick(BLOCK_SIZE);

	mesh_treasure = mesh_brick;
	mesh_treasure.tex_id = textureInit("resources/treasure.bmp", false, false);

	gen_mesh_magazines(meshes_magazine);
	{
		GLuint t = textureInit("resources/bullet.bmp", false, false);
		for (auto &i : meshes_magazine)
		{
			i.tex_id = t;
			i.texture_used = true;
		}
	}

	player.mouse_sensitivity = 10.0f;
	player.movement_speed = 2.0f;
	player.lock_cam_move_angles = true;
	player.radius = BLOCK_SIZE / 5.0f;
	
	globals.camera = &player;

	shot.movement_speed = 0.5f;
	shot.mouse_sensitivity = 10.0f;
	shot.lock_cam_move_angles = false;
	shot.radius = BLOCK_SIZE / 100.0f;
}

static void local_init(void)
{
	//
	// OpenGL settings
	// 

	glClearColor(0.2f, 0.2f, 0.4f, 0.0f);               // color-buffer clear colour
	glEnable(GL_CULL_FACE);  // disable draw of back face
	glCullFace(GL_BACK);
	glShadeModel(GL_SMOOTH);                        // set Gouraud shading

	glEnable(GL_DEPTH_TEST);                        // enable depth test  
	glPolygonMode(GL_FRONT, GL_FILL);       // enable polygon fill

	GLfloat light_position[] = { 0.0f, 0.0f, 45.0f, 1.0f };
	GLfloat light_direction[] = { 0.0, 0.0, -1.0 };
	GLfloat light_color[] = { 1.0f, 1.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_POSITION, light_position);			// light setup 
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 8.0);
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.5f);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_color);

	glEnable(GL_LIGHTING);
	
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	{
		//
		// Lua 
		//
		const std::string fn = "resources/script.lua";

		if (lua_init(fn) != EXIT_SUCCESS)
		{
			std::cerr << "Lua virtual Machine initialization error." << std::endl;
			finalize(EXIT_FAILURE);
		}
		std::cout << "[CPP," << glfwGetTime() << "] Lua script '" << fn << "' loaded succesfully." << std::endl;
	}
	
	{
		if (!glewIsExtensionSupported("GL_ARB_compatibility"))
			std::cerr << ":-(" << std::endl;

		//####===---       SHADERS                                            ---===###//
		std::vector<GLuint> shaders_ids;
		shaders_ids.push_back(compile_shader("resources/simple.vert", GL_VERTEX_SHADER));
		shaders_ids.push_back(compile_shader("resources/simple.frag", GL_FRAGMENT_SHADER));
		shader_id = link_shader(shaders_ids);
	}
}

void error_callback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

void fbsize_callback(GLFWwindow* window, int width, int height)
{
	// check for limit case (prevent division by 0)
	if (height == 0)
		height = 1;

	float ratio = (float)width / (float)height;

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
}
