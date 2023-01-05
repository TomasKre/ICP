// enable Z buffer test
glEnable(GL_DEPTH_TEST);

// ALL objects are non-transparent, cull back face of polygons 
glEnable(GL_CULL_FACE);


glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);




	// transformations
	// projection & viewport
	{
		int width, height;
		glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

		float ratio = static_cast<float>(width) / height;

		glm::mat4 projectionMatrix = glm::perspective(
			glm::radians(60.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
			ratio,			     // Aspect Ratio. Depends on the size of your window.
			0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
			20000.0f              // Far clipping plane. Keep as little as possible.
		);

		//set uniform for shaders - projection matrix
		glUniformMatrix4fv(glGetUniformLocation(id_prog_shader, "uP_m"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		// set visible area
		glViewport(0, 0, width, height);
	}

	// player & position
	glm::vec3 player_position(0.0f, 0.0f, 0.0f);

	// View matrix
	glm::mat4 v_m = glm::lookAt(player_position, //position of camera
								glm::vec3(0.0f,0.0f,0.0f), //where to look
								glm::vec3(0, 1, 0)  //UP direction
							);
	glUniformMatrix4fv(glGetUniformLocation(id_prog_shader, "uV_m"), 1, GL_FALSE, glm::value_ptr(v_m));

	// Model Matrix
	glm::mat4 m_m = glm::identity<glm::mat4>();
	glUniformMatrix4fv(glGetUniformLocation(id_prog_shader, "uM_m"), 1, GL_FALSE, glm::value_ptr(m_m));
	// =====================================================================================================

	// modify Model matrix and send to shaders
	// rotate slowly
	m_m = glm::rotate(m_m, glm::radians(100.0f * (float)glfwGetTime()), glm::vec3(0.0f, 0.1f, 0.0f));

	// =====================================================================================================
	if ((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS))
		player_position.x += 1.0;
	if ((glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS))
		player_position.x -= 1.0;
	if ((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS))
		player_position.z -= 1.0;
	if ((glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS))
		player_position.z += 1.0;
	// =====================================================================================================

