#pragma once

#include "GLStuff.h"
#include "AssimpModel.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class ModelRenderer : public BaseRenderer
{
public:
	ModelRenderer(int width, int height)
	{
		mModel.load("data/models/MyRectangler.dae"); //sport-car.dae");// MyRectangler.dae");

		mModel.getModelRange(xMin, xMax, yMin, yMax, zMin, zMax);
		xCenter = (xMin + xMax) / 2;
		yCenter = (yMin + yMax) / 2;
		zCenter = (zMin + zMax) / 2;

		R = std::max((xMax - xMin) / 2, (zMax - zMin) / 2);
		R = R*1.3 + 0.5;
		//int width, height;
		//glfwGetWindowSize(window, &width, &height);
		//float aspect = (float)width / (float)height;
		//projMat = glm::ortho(-200 * aspect, 200 * aspect, -200.0f, 200.0f, -100.f, 100.f);
		projMat = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 500.0f);
		//glmMatPrint<glm::mat4>(projMat, 4, "projMat");
		alpha = 0;
	}

	void renderer(float t)
	{
		alpha = alpha + 180 * 0.004;

		if (alpha >= 360) alpha = 0;
		float x = R * cos(alpha / 57.3);
		float y = 0.1*R + R * sin(alpha / 57.3);
		float z = R * sin(alpha / 57.3);
		viewMat = glm::lookAt(glm::vec3(x + xCenter, y + yCenter, z + zCenter), //Camera Position
			glm::vec3(xCenter, yCenter, zCenter), //Camera target point
			glm::vec3(0.0, 1.0, 0.0)  //Camera up direction
		);
		//printf("alpha=%.2f\n", alpha);

		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

		mModel.renderer(projMat, viewMat);
	}

private:
	AssimpModel mModel;
	glm::mat4 projMat, viewMat;
	float alpha;

	float xMin, xMax;
	float yMin, yMax;
	float zMin, zMax;
	float xCenter, yCenter, zCenter;
	float R;
};