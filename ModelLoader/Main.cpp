#define _CRT_SECURE_NO_WARNINGS
#include "SimpleRenderer.h"
#include "ModelRenderer.h"
//#include "Model.h"

#include <GL/glew.h>
//Include GLFW
#include <GLFW/glfw3.h>

//https://github.com/Madsy/Assimp-GL-Wrapper
//https://github.com/nothings/stb

static const int WIN_WIDTH = 1280;
static const int WIN_HEIGHT = 720;

int main(int argc, char** argv) {

	//Initialize GLFW
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	//Request an OpenGL 3.3 core context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Declare a window object
	GLFWwindow* window;

	//Create a window and create its OpenGL context
	//	window = glfwCreateWindow(1920, 1200, "OpenGL Compute Shader Particle System", glfwGetPrimaryMonitor(), NULL);
	window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "OpenGL System", NULL, NULL);

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

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	//Scene scene("data/modles/pandoras_box2.x");
	//Scene scene("data/modles/test.dae");


	try {
		//Scene scene("build/data/JaiquaFromXSI.dae");
		//Model scene("data/models/MyRectangler.dae");
		//Scene scene("build/data/TurbochiFromXSI.dae");
#if 0
		std::string animName("");
		if (argc == 3)
			animName = std::string(argv[2]);

		aiVector3D trans(0.0f, 0.0f, -2.0f);
		aiMatrix4x4 camera;
		aiMatrix4x4::Translation(trans, camera);
		AnimGLData* animation = scene.createAnimation(animName, camera);
		//AnimGLData* animation = scene.createAnimation(0, camera);
		if (!animation) {
			printf("Couldn't find animation \"%s\".\n", animName.c_str());
			glfwDestroyWindow(window);
			glfwTerminate();
			return 0;
		}

		AnimRenderer* renderer = new SimpleRenderer(WIN_WIDTH, WIN_HEIGHT);
		for (size_t i = 0; i < scene.getMeshCount(); ++i) {
			animation->addRenderer(renderer, i);
		}
#endif
		ModelRenderer *model = new ModelRenderer(WIN_WIDTH, WIN_HEIGHT);
		uint32_t frameCounter = 0;

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			double thisStarttime = glfwGetTime();
			//animation->render(thisStarttime);
			model->renderer(thisStarttime);
			glfwSwapBuffers(window);

			frameCounter++;

			double spendTime = glfwGetTime() - thisStarttime;

			char buf[128];
			sprintf(buf, "%.2f", spendTime * 1000);
			std::string windowTitle = "OpenGL Compute System (";

			windowTitle += buf;
			windowTitle += " ms)";
			const char* windowCaption = windowTitle.c_str();
			glfwSetWindowTitle(window, windowCaption);

			if (thisStarttime*32.0f >= 190.0f)
				glfwSetTime(0.0f);

		}
	}
	catch (std::exception& e) {
		//printf("Couldn't load file \"%s\"\n", scene.c_str());
		printf("Couldn't load file ");
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}