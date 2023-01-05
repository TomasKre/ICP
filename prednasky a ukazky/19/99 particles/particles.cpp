#define PART_CNT 1'000'000
#define ATTENUATION 0.95f

std::vector<glm::vec3> position(PART_CNT, glm::vec3(0.0f));
std::vector<glm::vec3> speed(PART_CNT, glm::vec3(0.0f));

//=====================================================================================================

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	static bool aa = false;
	static GLfloat point_size = 1.0f;

	if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
	{
		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case GLFW_KEY_P:  // Antialiasing, only for points (not applicable - we use MSAA, so allways on)
			std::cout << "Point AA switch" << std::endl;
			if (aa)
				glEnable(GL_POINT_SMOOTH);
			else
				glDisable(GL_POINT_SMOOTH);
			aa = !aa;
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
		default:
			break;
		}
	}
}

//=====================================================================================================

float random(float min, float max)
{
	return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

void physics(void)
{
	for (unsigned u = 0; u < PART_CNT; u++)
	{
		if ((fabs(speed[u].x) + fabs(speed[u].y) + fabs(speed[u].z)) <= 1.0)
		{
			position[u].x = 0;
			position[u].y = 100;
			position[u].z = 0;
			speed[u].x = random(-10, 10);
			speed[u].y = random(-30, 30);
			speed[u].z = random(-10, 10);
		}
		else {
			position[u].x += speed[u].x;
			position[u].y += speed[u].y;
			position[u].z += speed[u].z;

			if (position[u].y <= 0.0) {
				speed[u].x *= ATTENUATION;
				speed[u].y *= -ATTENUATION;
				speed[u].z *= ATTENUATION;
			}
			speed[u].y += 0.1f * (-9.8f);
		}
	}
}

//=====================================================================================================
// Vertex definition
struct vertex {
    glm::vec3 position;
};

// PLANE
std::vector<vertex> vertices = {
{glm::vec3(5000, 0, 5000)},
{glm::vec3(5000, 0, -5000)},
{glm::vec3(-5000, 0, -5000)},
{glm::vec3(-5000, 0, 5000)}
};

// Indices to draw a square using two triangles
std::vector<GLuint> indices = { 0,1,2,0,2,3 };

void main(void)
{
    ...
    vao_plane = {
        ... init VAO, VBO, EBO for floor plane
        usign vertices, indices
    }
    
    vao_particles = {
        ... init ONLY VAO, VBO (no EBO) for floor plane
        using position 
    }
    
    while (1) {

    	// PHYSICS
        physics();
    
    	// DRAW plane
        //... set material uniform to floor color...
        vao_plane.bind()
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    	// particles
        //...set material uniform to particle color...
        vao_particles.bind()
    	glDrawArrays(GL_POINTS, 0, PART_CNT);
    }
}
