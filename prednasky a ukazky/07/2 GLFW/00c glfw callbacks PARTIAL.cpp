void error_callback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void fbsize_callback(GLFWwindow* window, int width, int height)
{
	globals.width = width;
	globals.height = height;

	glMatrixMode(GL_PROJECTION);				// set projection matrix for following transformations
	glLoadIdentity();							// clear all transformations

	glOrtho(0, width, 0, height, -20000, 20000);  // set Orthographic projection

	glViewport(0, 0, width, height);			// set visible area
}