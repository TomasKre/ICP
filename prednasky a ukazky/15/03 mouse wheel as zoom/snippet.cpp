// Make Field-Of-View (FOV) global
struct s_globals {
...
	float fov = 60.0f;
...
};

//----------------------------------------------------------------------------
// Register callback in init()
...
glfwSetScrollCallback(window, scroll_callback);
...

//----------------------------------------------------------------------------
// Define callback 
void scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
	// standard wheel = yoffset
    globals.fov += 10*yoffset; // each step is offset of 1.0, too small to use directly -> multiplier 
	globals.fov = std::clamp(globals.fov, 20.0f, 170.0f); //limit to range 20...170
	update_canvas_size(); // reset projection
}

//----------------------------------------------------------------------------
// Modify update_canvas_size()
update_canvas_size()
{
...
	glm::mat4 projectionMatrix = glm::perspective(
		glm::radians(globals.fov), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
		ratio,			     // Aspect Ratio. Depends on the size of your window.
		0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
		20000.0f              // Far clipping plane. Keep as little as possible.
	);
...
} 
