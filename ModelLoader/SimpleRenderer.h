#pragma once

#include "GLStuff.h"
#include "AssimpModel.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>


class SimpleRenderer : public AnimRenderer
{
public:
	SimpleRenderer(int width, int height)
	{
		shader = createShaderProgram("data/shaders/shader.vs", "data/shaders/shader.fs");
		projection = glm::perspective(90.0f, 16.0f / 9.0f, 1.0f, 100.0f);
		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, width, height);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

		texture = loadTexture("data/models/texture.png");
	}

	void draw(int idx)
	{
		drawBegin(shader, idx);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		bindUniformSampler(shader, "sampler0", GL_TEXTURE0);
		int loc = glGetUniformLocation(shader, "projection");
		if (loc != -1)
			glUniformMatrix4fv(loc, 1, GL_TRUE, glm::value_ptr(projection));

		drawEnd(idx);
	}
private:
	GLuint shader;
	GLuint texture;
	glm::mat4 projection;
};