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

//mesh
mesh mesh_floor;
mesh mesh_brick;
mesh mesh_treasure;
mesh mesh_gun;

std::stack<glm::mat4> stack_modelview;

Avatar  player = { 5000.0f, 0.0f, 5000.0f, 0.0f };
Avatar  shot = { 0.0, 0.0, 0.0 };

int bullet = 5;					//bullet count 
int waitInterval = 0;			//min shooting interval
int cas = 200;
bool fireBullet = false;				//is fired?
double lastBulletUpdate = 0.0;
bool enableLight = true;

#define BRICK_SIZE				20.0f
#define TEXTURE_GROUND          0                  // texture names
#define TEXTURE_WALL1           1
#define TEXTURE_WALL2           2
#define TEXTURE_TREASURE        3
#define TEXTURE_BULLET			4
#define TEXTURE_GUN				5

GLuint textures[6];                                    // texture numbers

cv::Mat mapa = cv::Mat(20, 15, CV_8U);

//=====================================================================================================
//FORWARD DECLARATIONS
static void DrawScene(glm::mat4& mv_mat4);

void genLabyrinth(void);

void loadTextures(void);
//=====================================================================================================

//---------------------------------------------------------------------
// Random map gen
//---------------------------------------------------------------------
void genLabyrinth(void) {
	int i, j;
	cv::Point2i start, end;

	// C++ random numbers
	std::random_device r; // Seed with a real random value, if available
	std::default_random_engine e1(r());
	std::uniform_int_distribution<int> uniform_height(1, mapa.rows-1); // uniform distribution between int..int
	std::uniform_int_distribution<int> uniform_width(1, mapa.cols-1);
	std::uniform_int_distribution<int> uniform_block(0, 1);

	//gen start inside maze (excluding outer walls)
	start.x = uniform_width(e1);
	start.y = uniform_height(e1);

	//gen end different from start, inside maze (excluding outer walls) 
	do {
		end.x = uniform_width(e1);
		end.y = uniform_height(e1);
	} while ( start == end ); //check overlap

	std::cout << "Start: " << start << std::endl;
	std::cout << "End: " << end << std::endl;
		
	//inner maze 
	for (j = 0; j < mapa.rows; j++) {
		for (i = 0; i < mapa.cols; i++) {

			switch (uniform_block(e1))
			{
			case 0:
				mapa.at<uchar>(cv::Point(i, j)) = '.';
				break;
			case 1:
				mapa.at<uchar>(cv::Point(i, j)) = '#';
				break;
			default:
				break;
			}

			if ((i == start.x) && (j == start.y)) {
				mapa.at<uchar>(cv::Point(i, j)) = 'X';
			}
			if ((i == end.x) && (j == end.y)) {
				mapa.at<uchar>(cv::Point(i, j)) = 'e';
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

	//print map
	for (j = 0; j < mapa.rows; j++) {
		for (i = 0; i < mapa.cols; i++) {
			std::cout << mapa.at<uchar>(cv::Point(i, j));
		}
		std::cout << std::endl;
	}

	//set player position
	player.posX = (start.x * BRICK_SIZE) - BRICK_SIZE / 2.0;  //TODO: position...
	player.posZ = (start.y * BRICK_SIZE) - BRICK_SIZE / 2.0;
}

//---------------------------------------------------------------------
// Collision detection with walls
//---------------------------------------------------------------------	
bool collision(Avatar& avatar, cv::Mat& map)
{
	int kvadrantX, kvadrantZ;
	float avatar_posXV, avatar_posZV;
	int sloupPosX_min, sloupPosZ_min, sloupPosX_max, sloupPosZ_max;
	int kolize = false;

	//avatar quadrant
	kvadrantX = ceilf(avatar.posX / BRICK_SIZE);
	kvadrantZ = ceilf(avatar.posZ / BRICK_SIZE);

	//avatar position
	avatar_posXV = avatar.posX;
	avatar_posZV = avatar.posZ;
	avatar_posXV = abs(avatar_posXV - 3);
	avatar_posZV = abs(avatar_posZV - 3);

	//wall position
	sloupPosX_min = kvadrantX * (-BRICK_SIZE) - 00 + 2;
	sloupPosZ_min = kvadrantZ * (-BRICK_SIZE) - 00 + 2;
	sloupPosX_max = kvadrantX * (-BRICK_SIZE) + BRICK_SIZE - 1;
	sloupPosZ_max = kvadrantZ * (-BRICK_SIZE) + BRICK_SIZE - 1;

	if (map.at<uchar>(cv::Point(abs(kvadrantX), abs(kvadrantZ))) == '#') {
		if ((avatar_posXV < sloupPosX_max) && (avatar_posXV > sloupPosX_min) && (avatar_posZV < sloupPosZ_max) && (avatar_posZV > sloupPosZ_min)) {
			kolize = true;
		}
	}

	return kolize;
}

//---------------------------------------------------------------------
// Load and create textures
//---------------------------------------------------------------------
void loadTextures(void)
{
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glGenTextures(5, textures);
	//if (textureLoad(textures[TEXTURE_WALL1], "wall1.bmp"))    exit(0);
	//if (textureLoad(textures[TEXTURE_WALL2], "wall2.bmp"))    exit(0);
	//if (textureLoad(textures[TEXTURE_TREASURE], "treasure.bmp")) exit(0);
	//if (textureLoad(textures[TEXTURE_BULLET], "bullet.bmp"))   exit(0);
	//if (textureLoad(textures[TEXTURE_GUN], "gun.bmp"))      exit(0);
}



static void DrawScene(glm::mat4& mv_mat4)
{
	glm::vec3 color_red(1.0f, 0.0f, 0.0f);
	glm::vec3 color_green(0.0f, 1.0f, 0.0f);
	glm::vec3 color_blue(0.0f, 0.0f, 1.0f);
	glm::vec3 color_white(1.0f, 1.0f, 1.0f);
	glm::vec3 color_grey(0.7f, 0.7f, 0.7f);
	glm::vec3 color_black(0.0f, 0.0f, 0.0f);

	GLfloat color_R[] = { 1.0, 0.0, 0.0 };
	GLfloat color_G[] = { 0.0, 1.0, 0.0 };
	GLfloat color_lightGreen[] = { 0.5, 1.0, 0.5 };
	GLfloat color_B[] = { 0.0, 0.0, 1.0 };

	double current_time = glfwGetTime();
	if (current_time - lastBulletUpdate > 0.01) //each 10ms
	{
		lastBulletUpdate = current_time;

		if (fireBullet == 1) {
			avatarMoveForward(shot);
		}
		waitInterval = waitInterval - 3;
		cas = cas - 1;
	}

	//light on/off
	if (enableLight == 1)
	{
		glEnable(GL_LIGHT0);
	}
	else
	{
		glDisable(GL_LIGHT0);
	}

	// floor
	mesh_draw(mesh_floor);
	
	// gun
	{
		glm::mat4 mv;
		mv = glm::translate(mv, glm::vec3(0.0, 0.0, -3.0));
		mv = glm::rotate(mv,globals.camera->h_angle, glm::vec3(0.0f, 1.0f, 0.0f));
		glLoadMatrixf(glm::value_ptr(mv));
		
		mesh_draw(mesh_gun);
		
		glLoadMatrixf(glm::value_ptr(mv_mat4));
	}
	
	// bullets
	{

	}
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	static bool aa = false;
	static GLfloat point_size = 1.0f;
	Avatar a = *globals.camera;

	if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
	{
		switch (key) {
		case GLFW_KEY_ESCAPE: //fallthrough
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case GLFW_KEY_DOWN: //fallthrough
		case GLFW_KEY_S:
			a = avatarMoveBackward(*globals.camera);
			break;
		case GLFW_KEY_UP: //fallthrough
		case GLFW_KEY_W:
			a = avatarMoveForward(*globals.camera);
			break;
		case GLFW_KEY_LEFT: //fallthrough
		case GLFW_KEY_A:
			a = avatarMoveLeft(*globals.camera);
			break;
		case GLFW_KEY_RIGHT: //fallthrough
		case GLFW_KEY_D:
			a = avatarMoveRight(*globals.camera);
			break;
		case GLFW_KEY_PAGE_UP: //fallthrough
		case GLFW_KEY_R:
			a = avatarMoveUp(*globals.camera);
			break;
		case GLFW_KEY_PAGE_DOWN: //fallthrough
		case GLFW_KEY_F:
			a = avatarMoveDown(*globals.camera);;
			break;
		case GLFW_KEY_KP_ADD:
			globals.camera->movement_speed += 1.0f;
			break;
		case GLFW_KEY_KP_SUBTRACT:
			if (globals.camera->movement_speed > 1.0f)
				globals.camera->movement_speed -= 1.0f;
			break;
		case GLFW_KEY_L:
			enableLight = !enableLight;
			std::cout << "Light: " << enableLight << std::endl;
			break;
		default:
			break;
		}
		
		if (!collision(a, mapa))
			*globals.camera = a;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			glEnable(GL_LIGHT1);
			if (waitInterval < 1 && bullet != 0) {
				bullet = bullet - 1;
				fireBullet = 1;
				waitInterval = 100;
				shot.h_angle = globals.camera->h_angle;
				shot.posX = globals.camera->posX;
				shot.posY = globals.camera->posY;
			}
		}
		else {
			glDisable(GL_LIGHT1);
		}
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			if (bullet != 5) bullet = 5;
		}
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	static int first = 1;
	static int old_x;
	if (first) {
		old_x = xpos;
		first = 0;
	}
	else {
		globals.camera->h_angle = globals.camera->mouse_sensitivity * glm::radians(-xpos + old_x);
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
		{
			glm::mat4 mv_mat4;

			//keyboard + mouse movement from camera
			//mv_mat4 = glm::lookAt(glm::vec3(globals.camera->posX, globals.camera->posY, globals.camera->posZ), glm::vec3(globals.camera->posX + cosf(globals.camera->h_angle), globals.camera->posY, globals.camera->posZ + sinf(globals.camera->h_angle)), glm::vec3(0.0f, 1.0f, 0.0f));
			//mv_mat4 = glm::translate(mv_mat4, glm::vec3(0.0f, 5.0f, 0.0f)); //height of player
			glLoadMatrixf(glm::value_ptr(mv_mat4));

			glEnable(GL_TEXTURE_2D);
			glDisable(GL_LIGHTING);
			glBindTexture(GL_TEXTURE_2D, mesh_floor.textureID);
			float size = 200;
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); glVertex3f(0,0,-100);
			glTexCoord2f(1.0, 0.0); glVertex3f(size,0,-100);
			glTexCoord2f(1.0, 1.0); glVertex3f(size, size,-100);
			glTexCoord2f(0.0, 1.0); glVertex3f(0, size,-100);
			glEnd();


			// Draw everything
			//DrawScene(mv_mat4);
		}

		// Swap front and back buffers (Calls glFlush() inside)
		glfwSwapBuffers(globals.window);

		// Check OpenGL errors
		gl_check_error();

		// Poll for and process events
		glfwPollEvents();
	}
}

static void init_mesh(void)
{
	mesh_floor = gen_mesh_floor(mapa, BRICK_SIZE);
	mesh_brick = gen_mesh_brick(BRICK_SIZE);
	mesh_treasure = gen_mesh_treasure(BRICK_SIZE);
	mesh_gun = gen_mesh_gun();
}

static void init_gl(void)
{
	//
	// OpenGL settings
	// 
	const glm::vec3 dark_grey(0.3f, 0.3f, 0.3f);

	//lighting model setup
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(dark_grey));  //colour of default ambient light
	glShadeModel(GL_SMOOTH);   //Gouraud shading
	glEnable(GL_NORMALIZE);    //normalisation of EVERYTHING! Slower, but safe. 
	glEnable(GL_LIGHTING);

	// light0 setup 
	{
		const GLfloat light_position[] = { 0.0f, 0.0f, 45.0f, 1.0f };
		const GLfloat light_direction[] = { 0.0, 0.0, -1.0 };
		const GLfloat light_color[] = { 1.0f, 1.0f, 1.0f };

		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
		glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 8.0);
		glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.5f);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_color);
		glEnable(GL_LIGHT0);
	}

	// light1 setup 
	{
		const GLfloat light_position[] = { 0.0f, 0.0f, 45.0f, 1.0f };
		const GLfloat light_direction[] = { 0.0, 0.0, -1.0 };
		const GLfloat light_color[] = { 1.0f, 0.3f, 0.3f };

		glLightfv(GL_LIGHT1, GL_POSITION, light_position);
		glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light_direction);
		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 3.0);
		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 1.5f);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light_color);
	}

	//more lighting setup...
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);



	glClearColor(0.2, 0.2, 0.4, 0.0);               // color-buffer clear colour
	glDisable(GL_CULL_FACE);  // disable draw of back face
	glCullFace(GL_BACK);
	glShadeModel(GL_SMOOTH);                        // set Gouraud shading

	glEnable(GL_DEPTH_TEST);                        // enable depth test  
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);// prefer quality over speed
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);       // enable polygon fill
	glEnable(GL_POINT_SMOOTH);                      // antialiasing
	glEnable(GL_LINE_SMOOTH);

	glEnable(GL_LIGHTING);

	glEnable(GL_TEXTURE_2D);
}

static void init(void)
{
	init_glfw();
	init_glew();
	gl_print_info();

	init_mesh();
	init_gl();

	glEnable(GL_CULL_FACE);

	globals.camera = &player;
	player.mouse_sensitivity = 1 / 10.0f;
	player.movement_speed = 25.0f;

	shot.movement_speed = 40.0f;

	loadTextures();                                 // load all textures
	genLabyrinth();									// create labyrinth

	glEnable(GL_TEXTURE_2D);
}
//############################################################################################################################
//############################################################################################################################
int main(int argc, char* argv[])
{
	init();

	app_loop();

	finalize(EXIT_SUCCESS);
}
//############################################################################################################################
//############################################################################################################################
