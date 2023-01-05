void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	static GLfloat point_size = 1.0f;

	if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
	{
		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case GLFW_KEY_T:
			frame_based_animation = !frame_based_animation;
			if (frame_based_animation)
				std::cout << "frame-based animation" << std::endl;
			else
				std::cout << "time-based animation" << std::endl;
			break;
		case GLFW_KEY_KP_ADD:
			point_size += 1.0f;
			glPointSize(point_size);
			break;
		case GLFW_KEY_KP_SUBTRACT:
			if (point_size > 1.0f)
				point_size -= 1.0f;
			glPointSize(point_size);
			break;
		case GLFW_KEY_UP:
			particles.resize(particles.size() + 10000);
			std::cout << "Particles +10000: " << particles.size() << std::endl;
			break;
		case GLFW_KEY_DOWN:
			if (particles.size() > 0)
			{
				if (particles.size() > 10000)
					particles.resize(particles.size() - 10000);
				else
					particles.resize(0);
			}
			std::cout << "Particles -10000: " << particles.size() << std::endl;
			break;
		case GLFW_KEY_HOME:
			particles.resize(100000);
			std::cout << "Particles reinit: " << particles.size() << std::endl;
			break;
		case GLFW_KEY_END:
			particles.resize(1000);
			std::cout << "Particles reinit: " << particles.size() << std::endl;
			break;
		default:
			break;
		}
	}
}

