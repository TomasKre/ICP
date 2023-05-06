// C++ 
#include <iostream>
#include <chrono>
#include <numeric>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <thread>
#include <memory> //for smart pointers (unique_ptr)
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>

// OpenCV 
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler
#include <GL/glew.h> 
#include <GL/wglew.h> //WGLEW = Windows GL Extension Wrangler (change for different platform) 

// GLFW toolkit
#include <GLFW/glfw3.h>

// OpenGL math
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <irrklang/irrKlang.h>

void init_glew(void);
void init_glfw(void);
void error_callback(int error, const char* description);
void finalize(int code);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

std::string getProgramInfoLog(const GLuint obj);
std::string getShaderInfoLog(const GLuint obj);
std::string textFileRead(const std::string fn);

GLuint gen_tex(std::string filepath);
void tex_setup(int index);
void make_shader(std::string vertex_shader, std::string fragment_shader, GLuint* shader);

struct vertex {
  glm::vec3 position; // Vertex pos
  glm::vec3 color; // Color
};

// vertex with texture
struct tex_vertex {
  glm::vec3 position;
  glm::vec2 texcoord;
};

std::vector<tex_vertex> tex_vertices;

// create sound engine
irrklang::ISoundEngine* engine = irrklang::createIrrKlangDevice();

void va_setup(int index);
bool loadOBJ(const char* path, std::vector <vertex>& out_vertices, std::vector <GLuint>& indices, glm::vec3 color, glm::vec3 scale, glm::vec3 coords);

glm::vec3 check_collision(float x, float z);
std::array<bool, 3> check_objects_collisions(float x, float z);
void init_object_coords();

void setup_objects();

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

typedef struct image_data {
  cv::Point2f center;
  cv::Mat frame;
} image_data;

std::unique_ptr<image_data> image_data_shared;

typedef struct s_globals {
  GLFWwindow* window;
  int height;
  int width;
  double app_start_time;
  cv::VideoCapture capture;
  bool fullscreen = false;
  int x = 0;
  int y = 0;
  double last_fps;
  float fov = 90.0f;
} s_globals;

s_globals globals;

std::mutex img_access_mutex;
bool image_proccessing_alive;

// player & position
glm::vec3 player_position(-10.0f, 1.0f, -10.0f);
glm::vec3 looking_position(10.0f, 1.0f, 10.0f);
glm::vec3 up(0, 1, 0);

GLfloat Yaw = -90.0f;
GLfloat Pitch = 0.0f;;
GLfloat Roll = 0.0f;

GLfloat lastxpos = 0.0f;
GLfloat lastypos = 0.0f;
#define array_cnt(a) ((unsigned int)(sizeof(a) / sizeof(a[0])))

// movement and sound help variables
int step_delay = 0;
bool ouch_ready = true;
int move_count = 0;

// objects values
const int n_objects = 13;
GLuint VAO[n_objects];
GLuint VBO[n_objects];
GLuint EBO[n_objects];
std::vector<vertex> vertex_array[n_objects];
std::vector<GLuint> indices_array[n_objects];
glm::vec3 colors[n_objects];
glm::vec3 scales[n_objects];
glm::vec3 coordinates[n_objects];

// objects with collisions
struct coords {
  float min_x;
  float max_x;
  float min_z;
  float max_z;
};
const int n_col_obj = 9;
std::vector<vertex> col_obj[n_col_obj];
coords objects_coords[n_col_obj];

int main()
{

  std::mt19937_64 rng;
  // initialize the random number generator with time-dependent seed
  uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
  rng.seed(ss);
  // initialize a uniform distribution between 0 and 1
  std::uniform_real_distribution<double> unif(0, 1);

  init_glfw();
  init_glew();

  if (glfwExtensionSupported("GL_ARB_debug_output"))
  {
    glDebugMessageCallback(MessageCallback, 0);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    std::cout << "GL_DEBUG enabled." << std::endl;
  }

  // enable Z buffer test
  glEnable(GL_DEPTH_TEST);

  // ALL objects are non-transparent, cull back face of polygons 
  glEnable(GL_CULL_FACE);

  // create shaders
  GLuint prog_h;
  std::cout << "BASIC SHADER" << '\n';
  make_shader("resources/my.vert", "resources/my.frag", &prog_h);
  glUseProgram(prog_h);

  GLuint prog_tex;
  std::cout << "TEXTURE SHADER" << '\n';
  make_shader("resources/texture.vert", "resources/texture.frag", &prog_tex);

  // load objects
  setup_objects();

  // set callbacks
  glfwSetCursorPosCallback(globals.window, cursor_position_callback);
  glfwSetScrollCallback(globals.window, scroll_callback);
  glfwSetMouseButtonCallback(globals.window, mouse_button_callback);
  glfwSetKeyCallback(globals.window, key_callback);

  // frames and time
  int frame_cnt = 0;
  globals.last_fps = glfwGetTime();

  // transformations
  // projection & viewport
  int width, height;
  glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

  float ratio = static_cast<float>(width) / height;

  // set visible area
  glViewport(0, 0, width, height);

  GLuint texture_id = gen_tex("resources/tex/box.png");
  while (!glfwWindowShouldClose(globals.window)) {

    glm::mat4 projectionMatrix = glm::perspective(
      glm::radians(globals.fov), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90� (extra wide) and 30� (quite zoomed in)
      ratio,			     // Aspect Ratio. Depends on the size of your window.
      0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
      20000.0f              // Far clipping plane. Keep as little as possible.
    );

    //set uniform for shaders - projection matrix
    glUniformMatrix4fv(glGetUniformLocation(prog_h, "uP_m"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
      glUseProgram(prog_h);

      // Model Matrix
      glm::mat4 m_m = glm::identity<glm::mat4>();

      // modify Model matrix and send to shaders
      m_m = glm::scale(m_m, glm::vec3(2.0f));

      // p�ed�n� do shaderu
      glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));

      // View matrix
      glm::mat4 v_m = glm::lookAt(player_position, //position of camera
        glm::vec3(player_position + looking_position), //where to look
        up  //UP direction
      );
      // p�ed�n� do shaderu
      glUniformMatrix4fv(glGetUniformLocation(prog_h, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));

      // Use buffers
      for (int i = 1; i < n_objects - 3; i++) {
        glBindVertexArray(VAO[i]);
        glDrawElements(GL_TRIANGLES, indices_array[i].size(), GL_UNSIGNED_INT, 0);
      }

      // move sphere (sun)
      glm::mat4 temp = m_m;
      m_m = glm::rotate(m_m, glm::radians(10.0f * (float)glfwGetTime()), glm::vec3(1.0f, 0.0f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
      glBindVertexArray(VAO[10]);
      glDrawElements(GL_TRIANGLES, indices_array[10].size(), GL_UNSIGNED_INT, 0);
      m_m = temp;

      // rotate teapot
      temp = m_m;
      m_m = glm::rotate(m_m, glm::radians(20.0f * (float)glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
      glBindVertexArray(VAO[11]);
      glDrawElements(GL_TRIANGLES, indices_array[11].size(), GL_UNSIGNED_INT, 0);
      m_m = temp;

      // move teapot on edge
      temp = m_m;
      int edge = 21000;
      glm::vec3 change = (move_count < edge * 2) ?
        ((move_count < edge) ? glm::vec3(move_count * 0.001f, 0, 0) : glm::vec3(edge * 0.001f, 0, move_count * 0.001f - edge * 0.001f)) :
        ((move_count < edge * 3) ? glm::vec3(edge * 0.003f - move_count * 0.001f, 0, edge / 1000) : glm::vec3(0, 0, edge * 0.004f - move_count * 0.001f));
      m_m = glm::translate(m_m, change);
      glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
      glBindVertexArray(VAO[12]);
      glDrawElements(GL_TRIANGLES, indices_array[12].size(), GL_UNSIGNED_INT, 0);
      m_m = temp;
      move_count++;
      if (move_count == edge * 4) {
        move_count = 0;
      }
      glUniformMatrix4fv(glGetUniformLocation(prog_h, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));

      // textured object draw
      glUseProgram(prog_tex);
      glUniformMatrix4fv(glGetUniformLocation(prog_tex, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
      glUniformMatrix4fv(glGetUniformLocation(prog_tex, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));
      glUniformMatrix4fv(glGetUniformLocation(prog_tex, "uP_m"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

      //set texture unit
      glActiveTexture(GL_TEXTURE0);

      //send texture unit number to FS
      glUniform1i(glGetUniformLocation(prog_tex, "tex0"), 0);

      // draw object using VAO (Bind+DrawElements+Unbind)
      glBindVertexArray(VAO[0]);
      glBindTexture(GL_TEXTURE_2D, texture_id);
      glDrawElements(GL_TRIANGLES, indices_array[0].size(), GL_UNSIGNED_INT, 0);
      glUseProgram(prog_h);
    }
    // Prohodit buffery k vykreslen� a na��t�n�, zaznamenat eventy
    glfwSwapBuffers(globals.window);
    glfwPollEvents();

    // frames a time
    frame_cnt++;
    double now = glfwGetTime();

    // vyps�n� fps
    if ((now - globals.last_fps) > 1) {
      globals.last_fps = now;
      std::cout << "FPS: " << frame_cnt << std::endl;
      frame_cnt = 0;
    }
  }

  std::cout << "Program ended." << '\n';
  return (EXIT_SUCCESS);
}

void error_callback(int error, const char* description)
{
  std::cerr << "Error: " << description << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    finalize(EXIT_SUCCESS);
  //glfwSetWindowShouldClose(window, GLFW_TRUE);
  if (key == GLFW_KEY_W && action != GLFW_RELEASE)
    std::cout << 'W';
  if (key == GLFW_KEY_S && action != GLFW_RELEASE)
    std::cout << 'S';
  if (key == GLFW_KEY_A && action != GLFW_RELEASE)
    std::cout << 'A';
  if (key == GLFW_KEY_D && action != GLFW_RELEASE)
    std::cout << 'D';
  if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    if (globals.fullscreen) {
      glfwSetWindowMonitor(window, nullptr, globals.x, globals.y, 640, 480, 0);
      glViewport(0, 0, 640, 480);
      globals.fullscreen = false;
      glfwSetInputMode(globals.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else {
      glfwGetWindowSize(window, &globals.x, &globals.y);
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      glViewport(0, 0, mode->width, mode->height);
      globals.fullscreen = true;
      glfwSetInputMode(globals.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }
  float speed = 0.3f;

  if ((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)) {
    glm::vec3 xz = player_position + speed * glm::normalize(glm::cross(looking_position, up));
    player_position = check_collision(xz.x, xz.z);
  }
  if ((glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)) {
    glm::vec3 xz = player_position - speed * glm::normalize(glm::cross(looking_position, up));
    player_position = check_collision(xz.x, xz.z);
  }
  if ((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)) {
    float x = player_position.x + looking_position.x * speed;
    float z = player_position.z + looking_position.z * speed;
    player_position = check_collision(x, z);
  }
  if ((glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)) {
    float x = player_position.x - looking_position.x * speed;
    float z = player_position.z - looking_position.z * speed;
    player_position = check_collision(x, z);
  }
  std::cout << "Player position: " << player_position.x << " " << player_position.y << " " << player_position.z << " ";
}

glm::vec3 check_collision(float x, float z) {
  std::array<bool, 3> col = check_objects_collisions(x, z);
  if (col[0]) {
    //if object isn't in bounds of any object, move freely
    player_position.x = x;
    player_position.z = z;
    ouch_ready = true;
  }
  else {
    if (col[1]) {
      if (ouch_ready) {
        engine->play2D("resources/sounds/ouch.mp3");
        ouch_ready = false;
      }
      //if x step would not be in object bounds, move only on x axis
      player_position.x = x;
    }
    if (col[2]) {
      if (ouch_ready) {
        engine->play2D("resources/sounds/ouch.mp3");
        ouch_ready = false;
      }
      //if z step would not be in object bounds, move only on z axis
      player_position.z = z;
    }
  }
  if (step_delay == 0 && ouch_ready) { engine->play2D("resources/sounds/step1.mp3"); }
  if (step_delay == 8 && ouch_ready) { engine->play2D("resources/sounds/step2.mp3"); }
  if (step_delay++ == 16) { step_delay = 0; }
  return player_position;
}

std::array<bool, 3> check_objects_collisions(float x, float z) {
  std::array<bool, 3> col = { true, true, true };
  for (coords c : objects_coords) {
    //if x is in object bounds and z is in object bounds
    if (x > c.min_x && x < c.max_x && z > c.min_z && z < c.max_z) {
      col[0] = false;
      //if x step would be in object bounds
      if (player_position.x < c.min_x || player_position.x > c.max_x) {
        col[1] = false;
      }
      //if z step would be in object bounds
      if (player_position.z < c.min_z || player_position.z > c.max_z) {
        col[2] = false;
      }
      break;
    }
  }
  return col;
}

void init_object_coords() {
  //get min and max coords for objects (used in collision logic)
  for (int i = 0; i < n_col_obj; i++) {
    objects_coords[i].min_x = 999;
    objects_coords[i].max_x = -999;
    objects_coords[i].min_z = 999;
    objects_coords[i].max_z = -999;
    for (vertex v : col_obj[i]) {
      if (v.position[0] * 2 < objects_coords[i].min_x) {
        objects_coords[i].min_x = v.position[0] * 2;
      }
      if (v.position[0] * 2 > objects_coords[i].max_x) {
        objects_coords[i].max_x = v.position[0] * 2;
      }
      if (v.position[2] * 2 < objects_coords[i].min_z) {
        objects_coords[i].min_z = v.position[2] * 2;
      }
      if (v.position[2] * 2 > objects_coords[i].max_z) {
        objects_coords[i].max_z = v.position[2] * 2;
      }
    }
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    std::cout << "MOUSE_RIGHT" << '\n';
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    std::cout << "MOUSE_LEFT" << '\n';
  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    std::cout << "MOUSE_MIDDLE" << '\n';
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  std::cout << "mouse wheel(" << xoffset << ", " << yoffset << ")";
  globals.fov += 10 * -yoffset;
  if (globals.fov > 170.0f) {
    globals.fov = 170.0f;
  }
  if (globals.fov < 20.0f) {
    globals.fov = 20.0f;
  }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  //processMouseMovement(lastxpos-xpos, lastypos-ypos);
  std::cout << "cursor(" << xpos << ", " << ypos << ") ";

  Yaw += (xpos - lastxpos) / 5;
  Pitch += (lastypos - ypos) / 5;
  std::cout << "yp(" << Yaw << ", " << Pitch << ") ";

  if (true)
  {
    if (Pitch > 89.0f)
      Pitch = 89.0f;
    if (Pitch < -89.0f)
      Pitch = -89.0f;
  }

  looking_position.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
  looking_position.y = sin(glm::radians(Pitch));
  looking_position.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

  lastxpos = xpos;
  lastypos = ypos;
}

static void finalize(int code)
{
  // ...

  // Close OpenGL window if opened and terminate GLFW  
  if (globals.window)
    glfwDestroyWindow(globals.window);
  glfwTerminate();

  exit(code);
  // ...
}

static void init_glfw(void)
{
  //
  // GLFW init.
  //

  // set error callback first
  glfwSetErrorCallback(error_callback);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_SMOOTH);
  glEnable(GL_LINE_SMOOTH);

  // assume ALL objects are non-transparent 
  glEnable(GL_CULL_FACE);


  //initialize GLFW library
  int glfw_ret = glfwInit();
  if (!glfw_ret)
  {
    std::cerr << "GLFW init failed." << std::endl;
    finalize(EXIT_FAILURE);
  }

  // Shader based, modern OpenGL (3.3 and higher)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // only new functions
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // only old functions (for old tutorials etc.)

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

  glfwMakeContextCurrent(globals.window);                                        // Set current window.
  glfwGetFramebufferSize(globals.window, &globals.width, &globals.height);    // Get window size.
  //glfwSwapInterval(0);                                                        // Set V-Sync OFF.
  glfwSwapInterval(1);                                                        // Set V-Sync ON.


  globals.app_start_time = glfwGetTime();                                        // Get start time.
}

void init_glew(void) {
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
      exit(EXIT_FAILURE);
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
      exit(EXIT_FAILURE);
    }
    else
    {
      std::cout << "WGLEW successfully initialized platform specific functions." << std::endl;
    }
  }
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
  auto const src_str = [source]() {
    switch (source)
    {
    case GL_DEBUG_SOURCE_API: return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
    case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
    case GL_DEBUG_SOURCE_OTHER: return "OTHER";
    default: return "Unknown";
    }
  }();
  auto const type_str = [type]() {
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
    case GL_DEBUG_TYPE_MARKER: return "MARKER";
    case GL_DEBUG_TYPE_OTHER: return "OTHER";
    default: return "Unknown";
    }
  }();
  auto const severity_str = [severity]() {
    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
    case GL_DEBUG_SEVERITY_LOW: return "LOW";
    case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
    case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
    default: return "Unknown";
    }
  }();
  std::cout << "[GL CALLBACK]: " <<
    "source = " << src_str <<
    ", type = " << type_str <<
    ", severity = " << severity_str <<
    ", ID = '" << id << '\'' <<
    ", message = '" << message << '\'' << std::endl;
}

std::string textFileRead(const std::string fn) {
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  std::stringstream ss;

  file.open(fn);
  if (file.is_open()) {
    std::string content;
    ss << file.rdbuf();
  }
  else {
    std::cerr << "Error opening file: " << fn << std::endl;
    exit(EXIT_FAILURE);
  }
  return std::move(ss.str());
}

std::string getShaderInfoLog(const GLuint obj) {
  int infologLength = 0;
  std::string s;
  glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
  if (infologLength > 0) {
    std::vector<char> v(infologLength);
    glGetShaderInfoLog(obj, infologLength, NULL,
      v.data());
    s.assign(begin(v), end(v));
  }
  return s;
}

std::string getProgramInfoLog(const GLuint obj) {
  int infologLength = 0;
  std::string s;
  glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
  if (infologLength > 0) {
    std::vector<char> v(infologLength);
    glGetProgramInfoLog(obj, infologLength, NULL,
      v.data());
    s.assign(begin(v), end(v));
  }
  return s;
}

void va_setup(int index) {
  // Generate the VAO and VBO
  glGenVertexArrays(1, &VAO[index]);
  glGenBuffers(1, &VBO[index]);
  glGenBuffers(1, &EBO[index]);
  // Bind VAO (set as the current)
  glBindVertexArray(VAO[index]);
  // Bind the VBO, set type as GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, VBO[index]);
  // Fill-in data into the VBO
  glBufferData(GL_ARRAY_BUFFER, vertex_array[index].size() * sizeof(vertex), vertex_array[index].data(), GL_DYNAMIC_DRAW);
  // Bind EBO, set type GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[index]);
  // Fill-in data into the EBO
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_array[index].size() * sizeof(GLuint), indices_array[index].data(), GL_DYNAMIC_DRAW);
  // Set Vertex Attribute to explain OpenGL how to interpret the VBO
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex, position)));
  // Enable the Vertex Attribute 0 = position
  glEnableVertexAttribArray(0);
  // Set end enable Vertex Attribute 1 = Texture Coordinates
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0 + offsetof(vertex, color)));
  glEnableVertexAttribArray(1);
  // Bind VBO and VAO to 0 to prevent unintended modification
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool loadOBJ(const char* path, std::vector <vertex>& out_vertices, std::vector <GLuint>& indices, glm::vec3 color, glm::vec3 scale, glm::vec3 coords) {
  std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
  std::vector< glm::vec3 > temp_vertices;
  std::vector< glm::vec2 > temp_uvs;
  std::vector< glm::vec3 > temp_normals;

  out_vertices.clear();
  indices.clear();

  FILE* file;
  fopen_s(&file, path, "r");
  if (file == NULL) {
    printf("Impossible to open the file !\n");
    return false;
  }
  int index = 0;
  while (1) {

    char lineHeader[128];
    int res = fscanf_s(file, "%s", lineHeader, array_cnt(lineHeader));
    if (res == EOF) {
      break;
    }

    if (strcmp(lineHeader, "v") == 0) {
      glm::vec3 vertex;
      fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
      temp_vertices.push_back(vertex);
    }
    else if (strcmp(lineHeader, "vt") == 0) {
      glm::vec2 uv;
      fscanf_s(file, "%f %f\n", &uv.y, &uv.x);
      temp_uvs.push_back(uv);
    }
    else if (strcmp(lineHeader, "vn") == 0) {
      glm::vec3 normal;
      fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
      temp_normals.push_back(normal);
    }
    else if (strcmp(lineHeader, "f") == 0) {
      std::string vertex1, vertex2, vertex3;
      unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
      int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
      if (matches != 9) {
        printf("File can't be read by simple parser :( Try exporting with other options\n");
        return false;
      }
      vertexIndices.push_back(vertexIndex[0]);
      vertexIndices.push_back(vertexIndex[1]);
      vertexIndices.push_back(vertexIndex[2]);
      for (int j = 0; j < 6; j++)
      {
        indices.push_back(index + j);
      }
      index += 6;
    }
  }

  // unroll from indirect to direct vertex specification
  // sometimes not necessary, definitely not optimal

  for (unsigned int u = 0; u < vertexIndices.size(); u++) {
    unsigned int vertexIndex = vertexIndices[u];
    glm::vec3 vertex = coords + (temp_vertices[vertexIndex - 1] * scale);
    out_vertices.push_back({ vertex, color });
  }

  fclose(file);
  return true;
}

void setup_objects() {
  // TODO v�bec netu�im jak to m� fungovat a kdy� n�co zm�nim tak ten trojuheln�k zmiz� nebo je divnej
  tex_vertices.push_back({ {-10.0f, -10.0f, 0.0f }, glm::vec2(-10.0f, -10.0f) });
  tex_vertices.push_back({ {-10.0f, -9.0f, 0.0f }, glm::vec2(-10.0f, -9.0f) });
  tex_vertices.push_back({ { -9.0f, -10.0f, 0.0f}, glm::vec2(-9.0f, -10.0f) });
  tex_vertices.push_back({ { 10.0f, -1.0f, -10.0f}, glm::vec2(0.0f, 0.0f) });
  indices_array[0] = { 2, 1, 0 };

  tex_setup(0);

  //setup color, scale and coordinates for object
  colors[1] = { 0.3, 0.3, 0.3 };
  scales[1] = { 1, 1, 1 };
  coordinates[1] = { -10.5, -0.5, 10.5 };
  //load object from file
  loadOBJ("resources/obj/cube.obj", vertex_array[1], indices_array[1], colors[1], scales[1], coordinates[1]);
  //setup vertex array
  va_setup(1);

  colors[2] = { 0.3, 0.3, 0.3 };
  scales[2] = { 1, 1, 1 };
  coordinates[2] = { 10.5, -0.5, -10.5 };
  loadOBJ("resources/obj/cube.obj", vertex_array[2], indices_array[2], colors[2], scales[2], coordinates[2]);

  va_setup(2);


  colors[3] = { 0.3, 0.3, 0.3 };
  scales[3] = { 1, 1, 20 };
  coordinates[3] = { -10.5, -0.5, 0 };
  loadOBJ("resources/obj/cube.obj", vertex_array[3], indices_array[3], colors[3], scales[3], coordinates[3]);

  va_setup(3);

  colors[4] = { 0.3, 0.3, 0.3 };
  scales[4] = { 1, 1, 20 };
  coordinates[4] = { 10.5, -0.5, 0 };
  loadOBJ("resources/obj/cube.obj", vertex_array[4], indices_array[4], colors[4], scales[4], coordinates[4]);

  va_setup(4);

  colors[5] = { 0.3, 0.3, 0.3 };
  scales[5] = { 20, 1, 1 };
  coordinates[5] = { 0, -0.5, -10.5 };
  loadOBJ("resources/obj/cube.obj", vertex_array[5], indices_array[5], colors[5], scales[5], coordinates[5]);

  va_setup(5);

  colors[6] = { 0.3, 0.3, 0.3 };
  scales[6] = { 20, 1, 1 };
  coordinates[6] = { 0, -0.5, 10.5 };
  loadOBJ("resources/obj/cube.obj", vertex_array[6], indices_array[6], colors[6], scales[6], coordinates[6]);

  va_setup(6);

  colors[7] = { 0.3, 0.3, 0.3 };
  scales[7] = { 1, 1, 1 };
  coordinates[7] = { 10.5, -0.5, 10.5 };
  loadOBJ("resources/obj/cube.obj", vertex_array[7], indices_array[7], colors[7], scales[7], coordinates[7]);

  va_setup(7);

  colors[8] = { 0.3, 0.3, 0.3 };
  scales[8] = { 1, 1, 1 };
  coordinates[8] = { -10.5, -0.5, -10.5 };
  loadOBJ("resources/obj/cube.obj", vertex_array[8], indices_array[8], colors[8], scales[8], coordinates[8]);

  va_setup(8);

  colors[9] = { 0.5, 0, 0.5 };
  scales[9] = { 2, 1, 2 };
  coordinates[9] = { 0, -0.5, 0 };

  loadOBJ("resources/obj/cube.obj", vertex_array[9], indices_array[9], colors[9], scales[9], coordinates[9]);

  va_setup(9);

  colors[10] = { 1, 1, 1 };
  scales[10] = { 2, 2, 2 };
  coordinates[10] = { -20, 15, -20 };
  loadOBJ("resources/obj/sphere.obj", vertex_array[10], indices_array[10], colors[10], scales[10], coordinates[10]);

  va_setup(10);

  colors[11] = { 1, 0.1, 0.1 };
  scales[11] = { 0.1, 0.1, 0.1 };
  coordinates[11] = { 7, 3, 7 };

  loadOBJ("resources/obj/teapot.obj", vertex_array[11], indices_array[11], colors[11], scales[11], coordinates[11]);

  va_setup(11);

  colors[11] = { 1, 0.1, 0.1 };
  scales[11] = { 0.1, 0.1, 0.1 };
  coordinates[11] = { 7, 3, 7 };

  loadOBJ("resources/obj/teapot.obj", vertex_array[11], indices_array[11], colors[11], scales[11], coordinates[11]);

  va_setup(11);

  colors[12] = { 0.1, 0.1, 1.0 };
  scales[12] = { 0.1, 0.1, 0.1 };
  coordinates[12] = { -10.5, 0.0, -10.5 };

  loadOBJ("resources/obj/teapot.obj", vertex_array[12], indices_array[12], colors[12], scales[12], coordinates[12]);

  va_setup(12);

  //choose objects with collisions
  int j = 0;
  for (int i : {1, 2, 3, 4, 5, 6, 7, 8, 9}) {
    col_obj[j] = vertex_array[i];
    j++;
  }
  init_object_coords();
}

GLuint gen_tex(std::string filepath)
{
  GLuint ID;
  cv::Mat image = cv::imread(filepath);

  // Generates an OpenGL texture object
  glGenTextures(1, &ID);

  // Assigns the texture to a Texture Unit
  glBindTexture(GL_TEXTURE_2D, ID);

  // Texture data alignment for transfer (single byte = basic, slow, but safe option; usually not necessary) 
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, image.data);
  int compressed;
  GLint internalformat, compressed_size;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &compressed);
  /* if the compression has been successful */
  if (compressed == GL_TRUE)
  {
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &compressed_size);
    std::cout << "ORIGINAL: " << image.total() * image.elemSize() << " COMPRESSED: " << compressed_size << " INTERNAL FORMAT: " << internalformat << std::endl;
  }

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  // Configures the way the texture repeats
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  glGenerateMipmap(GL_TEXTURE_2D);

  // Unbinds the OpenGL Texture object so that it can't accidentally be modified
  glBindTexture(GL_TEXTURE_2D, 0);

  return ID;
}

void tex_setup(int index) {
  // Generate the VAO and VBO
  glGenVertexArrays(1, &VAO[index]);
  glGenBuffers(1, &VBO[index]);
  glGenBuffers(1, &EBO[index]);
  // Bind VAO (set as the current)
  glBindVertexArray(VAO[index]);
  // Bind the VBO, set type as GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, VBO[index]);
  // Fill-in data into the VBO
  glBufferData(GL_ARRAY_BUFFER, tex_vertices.size() * sizeof(tex_vertex), tex_vertices.data(), GL_DYNAMIC_DRAW);
  // Bind EBO, set type GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[index]);
  // Fill-in data into the EBO
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_array[0].size() * sizeof(GLuint), indices_array[0].data(), GL_DYNAMIC_DRAW);
  // Set Vertex Attribute to explain OpenGL how to interpret the VBO
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)(0 + offsetof(tex_vertex, position)));
  // Enable the Vertex Attribute 0 = position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)(0 + offsetof(tex_vertex, texcoord)));
  glEnableVertexAttribArray(1);
  // Bind VBO and VAO to 0 to prevent unintended modification
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void make_shader(std::string vertex_shader, std::string fragment_shader, GLuint* shader) {
  GLuint VS_h, FS_h, prog_h;
  VS_h = glCreateShader(GL_VERTEX_SHADER);
  FS_h = glCreateShader(GL_FRAGMENT_SHADER);

  // vert
  std::string VSsrc = textFileRead(vertex_shader);
  const char* VS_string = VSsrc.c_str();
  // frag
  std::string FSsrc = textFileRead(fragment_shader);
  const char* FS_string = FSsrc.c_str();
  glShaderSource(VS_h, 1, &VS_string, NULL);
  glShaderSource(FS_h, 1, &FS_string, NULL);

  // compile and use shaders
  glCompileShader(VS_h);
  getShaderInfoLog(VS_h);
  glCompileShader(FS_h);
  getShaderInfoLog(FS_h);
  prog_h = glCreateProgram();
  glAttachShader(prog_h, VS_h);
  glAttachShader(prog_h, FS_h);
  glLinkProgram(prog_h);
  getProgramInfoLog(prog_h);
  *shader = prog_h;

  // check if vertex shader, fragment shader compiled successfuly and program linked
  GLint success = 0;
  std::cout << "Success false = " << GL_FALSE << std::endl;
  glGetShaderiv(VS_h, GL_COMPILE_STATUS, &success);
  std::cout << "Vertex shader " << success << std::endl;
  glGetShaderiv(FS_h, GL_COMPILE_STATUS, &success);
  std::cout << "Fragment shader " << success << std::endl;
  glGetProgramiv(prog_h, GL_LINK_STATUS, &success);
  std::cout << "Program linking " << success << std::endl;
}