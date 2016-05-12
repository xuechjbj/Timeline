#define GLEW_STATIC 
#include <GL/glew.h>
//Include GLFW
#include <GLFW/glfw3.h>

//Include the standard C++ headers
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	//Set the error callback
	//glfwSetErrorCallback(error_callback);

	//Initialize GLFW
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	//Request an OpenGL 4.3 core context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Declare a window object
	GLFWwindow* window;

	//Create a window and create its OpenGL context
	//	window = glfwCreateWindow(1920, 1200, "OpenGL Compute Shader Particle System", glfwGetPrimaryMonitor(), NULL);
	window = glfwCreateWindow(1280, 720, "OpenGL System", NULL, NULL);

	//If the window couldn't be created
	if (!window)
	{
		fprintf(stderr, "Failed to open GLFW window.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	//glfwSetKeyCallback(window, key_callback);
	//glfwSetWindowSizeCallback(window, window_size_callback);

	//This function makes the context of the specified window current on the calling thread. 
	glfwMakeContextCurrent(window);

	//Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	//If GLEW hasn't initialized
	if (err != GLEW_OK)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return -1;
	}

	// Output some info on the OpenGL implementation
	const GLubyte* glvendor = glGetString(GL_VENDOR);
	const GLubyte* glrenderer = glGetString(GL_RENDERER);
	const GLubyte* glversion = glGetString(GL_VERSION);

	fprintf(stderr, "Vendor: %s\n", glvendor);
	fprintf(stderr, "Renderer: %s\n", glrenderer);
	fprintf(stderr, "Version: %s\n", glversion);

	return 0;
}

