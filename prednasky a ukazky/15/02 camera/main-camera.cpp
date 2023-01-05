
// OPTION 1
#include <unordered_set>
std::unordered_set<int> keys;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    GLuint new_subroutine;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);


    if (action == GLFW_PRESS)
        keys.insert(key);
    else if (action == GLFW_RELEASE)
        keys.erase(key);
}

void process_input()
{
    double delta_t = last_frame_time - glfwGetTime();

    if (keys.count(GLFW_KEY_A) == 1)
    { 
        camera.ProcessInput(Camera::direction::LEFT, delta_t);
    }
    // process all keys...
    
    keys.clear();
}


// OPTION 2
void process_input()
{
    double delta_t = last_frame_time - glfwGetTime();

    if (glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS)
    {
        camera.ProcessInput(Camera::direction::LEFT, delta_t);
    }
    // process all keys


}

