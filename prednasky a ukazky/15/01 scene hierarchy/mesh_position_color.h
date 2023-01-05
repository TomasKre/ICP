
struct vertex_p_c {
    glm::vec3 position;
    glm::vec3 color;
};


class mesh_p_c {
public:
    std::vector<vertex_p_c> vertices;
    std::vector<GLuint> indices;
    GLuint VAO;
    GLenum primitive;
    GLuint shader_id;

    mesh_p_c(GLuint shader_type, std::vector<vertex_p_c>& vertices, std::vector<GLuint>& indices)
    :vertices(vertices), indices(indices), shader_id(shader_id)
    {
        GLuint VAO;
        GLuint VBO, EBO;
    
        // Generate the VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        // Bind VAO (set as the current)
        glBindVertexArray(VAO);
        // Bind the VBO, set type as GL_ARRAY_BUFFER
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Fill-in data into the VBO
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_p_c), vertices.data(), GL_STATIC_DRAW);
        // Bind EBO, set type GL_ELEMENT_ARRAY_BUFFER
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // Fill-in data into the EBO
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        // Set Vertex Attribute to explain OpenGL how to interpret the VBO
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_p_c), (void*)(0 + offsetof(vertex_p_c, position)));
        // Enable the Vertex Attribute 0 = position
        glEnableVertexAttribArray(0);
        // Set end enable Vertex Attribute 1 = Vertex Colors
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_p_c), (void*)(0 + offsetof(vertex_p_c, color)));
        glEnableVertexAttribArray(1);
    
        // Bind VBO and VAO to 0 to prevent unintended modification
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
        this->VAO = VAO;
        primitive = GL_TRIANGLES;
    }
    
    void draw(const glm::mat4 & M, const glm::mat4& V, const glm::mat4& P) {
        glUseProgram(shader_id);
        glUniformMatrix4fv(glGetUniformLocation(shader_id, "uP_m"), 1, GL_FALSE, glm::value_ptr(P));
        glUniformMatrix4fv(glGetUniformLocation(shader_id, "uV_m"), 1, GL_FALSE, glm::value_ptr(V));
        glUniformMatrix4fv(glGetUniformLocation(shader_id, "uM_m"), 1, GL_FALSE, glm::value_ptr(M));

        glBindVertexArray(VAO);
        glDrawElements(primitive, indices.size(), GL_UNSIGNED_INT, 0);
    }
};
