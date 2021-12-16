/*
 * Copyright © 2020 Antonin Stefanutti <antonin.stefanutti@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <GLES3/gl3.h>

#include "common.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint vao;
GLint iBlend;
GLint iRes;
GLint iTex1;
GLint iTex2;
GLuint tex1;
GLuint tex2;

static const char *shadertoy_vs =
		"attribute vec3 position;                            \n"
		"varying vec2 vertexUV;                              \n"
		"uniform vec2 iRes;                                  \n"
		"void main()                                         \n"
		"{                                                   \n"
		"    gl_Position = vec4(position, 1.0);              \n"
		"    vertexUV = (position.xy + vec2(1.0, 1.0)) * 0.5;\n"
		"    vertexUV.x *= iRes.y/iRes.x;                    \n"
		"}                                                   \n";

static const char *shadertoy_fs =
		"precision mediump float;                                    \n"
		"varying vec2 vertexUV;                                      \n"
		"uniform float iBlend;                                       \n"
		"uniform sampler2D texture1;                                 \n"
		"uniform sampler2D texture2;                                 \n"
		"                                                            \n"
		"void main()                                                 \n"
		"{                                                           \n"
		"    gl_FragColor = texture2D(texture1, vertexUV);           \n"
		"}                                                           \n";

static const GLfloat vertices[] = {
	 1.0,  1.0, 0.0,  1.0, 1.0,
     1.0, -1.0, 0.0,  1.0, 0.0,
    -1.0, -1.0, 0.0,  0.0, 0.0,
    -1.0,  1.0, 0.0,  0.0, 1.0,
};

static const GLuint indices[] = {
	0, 1, 3,
    1, 2, 3,
};

static void load_textures() {
	// printf("Loading textures\n");
	// unsigned int x, y, n;
	// unsigned char* data = stbi_load("/home/pi/test.bmp", &x, &y, &n, 3);

	// glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, tex1);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	// glActiveTexture(GL_TEXTURE1);
	// glBindTexture(GL_TEXTURE_2D, tex2);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

uint64_t picture_start = 0;

static void check_error() {
	if (glGetError() != 0) {
		printf("Uh oh error!\n");
	}
}

static void draw_shadertoy() {
	

	// double time = (get_time_ns() - picture_start) / (double) NSEC_PER_SEC;

	// if (picture_start == 0 || time > 60) {
	// 	load_textures();
	// 	picture_start = get_time_ns();
	// 	time = (get_time_ns() - picture_start) / (double) NSEC_PER_SEC;
	// }
	// glUniform1f(iBlend, time / 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(vao);
	// printf("Drawing\n");
	
	// glDrawArrays(GL_TRIANGLES, 0, 6);
	glDrawElements(GL_TRIANGLES, sizeof(indices), GL_UNSIGNED_INT, 0);
}

int init_shadertoy(const struct gbm *gbm, struct egl *egl) {
	int ret;
	GLuint program, vbo, ebo;

	ret = create_program(shadertoy_vs, shadertoy_fs);
	if (ret < 0) {
		printf("failed to create program\n");
		return -1;
	}

	program = ret;

	ret = link_program(program);
	if (ret) {
		printf("failed to link program\n");
		return -1;
	}

	glViewport(0, 0, gbm->width, gbm->height);


	stbi_set_flip_vertically_on_load(1);

	unsigned int x, y, n;
	unsigned char* data = stbi_load("/home/pi/test2.jpg", &x, &y, &n, 4);

	glGenTextures(1, tex1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_NV);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_NV);

	float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_NV, borderColor);  

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	glEnableVertexAttribArray(0);
	// glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 5 * sizeof(float), 2 * sizeof(float));
	// glEnableVertexAttribArray(1);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex1);

	glUseProgram(program);

	iBlend = glGetUniformLocation(program, "iBlend");
	iRes = glGetUniformLocation(program, "iRes");
	iTex1 = glGetUniformLocation(program, "texture1");
	iTex2 = glGetUniformLocation(program, "texture2");

	glUniform1i(iTex1, 0);
	glUniform1i(iTex2, 1);
	glUniform2f(iRes, x, y);

	egl->draw = draw_shadertoy;

	return 0;
}
