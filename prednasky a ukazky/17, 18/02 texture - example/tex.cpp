int main( int argc, char * argv[])
{
    ...
    init();
    ...
    VAO.init(GL_TRIANGLES,vertices,indices);
    texture_id = gen_tex("mytexture.png");
    ...
    while(1) {
        ...
        VAO.draw();
        ...
        glfwSwapBuffers();
    }
    ...
}
//-----------------------------------------------------------------------------
void init( void )
{
    glEnable( GL_DEPTH_TEST ); 
    glEnable( GL_POLYGON_SMOOTH );
    glEnable( GL_LINE_SMOOTH );

    // assume ALL objects are non-transparent 
    glEnable( GL_CULL_FACE );
}
//-----------------------------------------------------------------------------
// FLOOR

// Vertex definition
struct vertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

// Vertices + texture coordinates
std::vector<vertex> vertices = {
{glm::vec3(5000, 0, 5000),glm::vec2(0.0f, 0.0f)},
{glm::vec3(5000, 0, -5000),glm::vec2(1.0f, 0.0f)},
{glm::vec3(-5000, 0, -5000),glm::vec2(1.0f, 1.0f)},
{glm::vec3(-5000, 0, 5000),glm::vec2(0.0f, 1.0f)}
};

// Indices to draw a square using two triangles
std::vector<GLuint> indices = { 0,1,2,0,2,3 };

// Image used as a texture
cv::Mat image;

GLuint texture_id;

GLuint gen_tex(std::string filepath) 
{
    GLuint ID;
    cv::Mat image = cv::imread(filepath, cv::IMREAD_UNCHANGED); // Read with (potential) Alpha
    if (image.channels()!=4) exit(-1);  // Check the image, we want Alpha in this example    
    
    // Generates an OpenGL texture object
    glGenTextures(1, &ID);
    
    // Assigns the texture to a Texture Unit
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Texture data alignment for transfer (single byte = basic, slow, but safe option; usually not necessary) 
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);        

    // Assigns the image to the OpenGL Texture object
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.cols, image.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.data);

    // Configures the type of algorithm that is used to make the image smaller or bigger
    // nearest neighbor - ugly & fast 
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    // bilinear - nicer & slower
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    // MIPMAP filtering + automatic MIPMAP generation - nicest, needs more memory. Notice: MIPMAP is only for image minifying.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // bilinear magnifying
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear minifying
    glGenerateMipmap(GL_TEXTURE_2D);  //Generate mipmaps now.

    // Configures the way the texture repeats
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Unbinds the OpenGL Texture object so that it can't accidentally be modified
    glBindTexture(GL_TEXTURE_2D, 0);
    return ID;
}


void VAO.draw(void)
{

    // This is necessary for dynamic texture (=video)
    // resize/crop to square image and size of 2^n in separate thread (camera thread)
    // If camera has new image, load texture to GPU, select proper format...
    
    //if (new_img)
    //{
    //glBindTexture(ID);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.cols, image.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image.data);
    //} 
    
    //activate shader with texturing support        
    basic_tex_shader.activate();
    
    // send Model, View and Projection matrix
    glUniformMatrix4fv(glGetUniformLocation(basic_tex_shader.ID, "uM_m"), 1, GL_FALSE, glm::value_ptr(mv_m));
    glUniformMatrix4fv(glGetUniformLocation(basic_tex_shader.ID, "uV_m"), 1, GL_FALSE, glm::value_ptr(mv_m));
    glUniformMatrix4fv(glGetUniformLocation(basic_tex_shader.ID, "uP_m"), 1, GL_FALSE, glm::value_ptr(mv_m));

    //set texture unit
    glActiveTexture(GL_TEXTURE0);

    //send texture unit number to FS
    glUniform1i(glGetUniformLocation(basic_tex_shader.ID, "tex0"), 0);
    
    // draw object using VAO (Bind+DrawElements+Unbind)
    VAO.bind();
    glBindTexture(GL_TEXTURE_2D, texture_id); 
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    VAO.unbind();    
    
}
