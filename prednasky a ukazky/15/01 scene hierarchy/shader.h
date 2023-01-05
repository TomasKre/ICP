
class Shader {
public:
	GLuint ID;
	Shader(const std::string& vs_path, const std::string& fs_path) {
		//load
		//compile
		//link
	}

	void activate(void) { 
		//use
	};
	void destroy(void) {
		//delete
	}

private:
	std::string getShaderInfoLog(const GLuint obj); 
	std::string getProgramInfoLog(const GLuint obj);
	std::string textFileRead(const std::string& fn);

};