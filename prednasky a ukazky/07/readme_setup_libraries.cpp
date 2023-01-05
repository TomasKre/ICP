/*
* In your project directory:
*	create dirs: include, lib, bin
* 
*   Add to project paths:
*   VC++ directories -> Executable dirs:    $(ProjectDir)bin
*   				 -> Include dirs:		$(ProjectDir)include
*					 -> Library dirs:		$(ProjectDir)lib
*
* Download current GLFW (compiled for Windows x64: glfw-3.3.2.bin.WIN64.zip) from https://www.glfw.org/download.html
*	Extract from ZIP:
*		From dir: glfw-3.3.2.bin.WIN64\lib-vc2019\
			glfw3.dll into $(ProjectDir)bin
			*.lib	  into $(ProjectDir)lib
*		From dir glfw-3.3.2.bin.WIN64\include 
			dir GLFW and its content into $(projectDir)include
*		
* Download current GLEW (binary for 32&64 bit Windows: glew-2.1.0-win32.zip) from http://glew.sourceforge.net/
*	Extract from ZIP:
*		From dir: glew-2.1.0\bin\Release\x64
*			glew32.dll into $(ProjectDir)bin
*		From dir: glew-2.1.0\lib\Release\x64	
			*.lib	  into $(ProjectDir)lib
*		From dir: glew-2.1.0\include\
*			dir GL and its content into $(projectDir)include
* 
* Download current GLM (header only library) from https://glm.g-truc.net
*   Extract from ZIP:
*		From dir: glm-0.9.9.8
*			dir glm and its content into $(projectDir)include
* 
* All configurations 
*   Project -> Properties -> Debugging -> Environment
*       PATH=%PATH%;$(ProjectDir)bin; 
* 
* Project -> Properties -> Linker -> Input -> Additional dependencies
*   opengl32.lib;glew32.lib;glfw3dll.lib
* 
* 
*/

/* ========================================================================= */
// C++ 
#include <iostream>
#include <chrono>

// OpenCV 
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler
#include <GL/glew.h> 
#include <GL/wglew.h> //WGLEW = Windows GL Extension Wrangler (change for different platform) 

// GLFW toolkit
#include <GLFW/glfw3.h>

// OpenGL math
#include <glm/glm.hpp>

