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
#include <dirent.h>

#include <GLES3/gl3.h>

#include "common.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint vao;
GLint iBlend;
GLint i;
GLint iTex1;
GLint iTex2;
GLuint tex1;
GLuint tex2;
int tx;
int ty;

static const char *shadertoy_vs =
	"attribute vec3 position;                                    \n"
	"varying vec2 vertexUV;                                      \n"
	"uniform vec2 i;                                             \n"
	"uniform vec2 t;                                             \n"
	"void main()                                                 \n"
	"{                                                           \n"
	"    gl_Position = vec4(position, 1.0);                      \n"
	"    vertexUV = (position.xy + vec2(1.0, 1.0)) * 0.5;        \n"
	"    if (t.x/t.y > i.x/i.y) {                                \n"
	"        vertexUV.x -= (t.x-t.y/i.y*i.x)/(2.0*t.x);          \n"
    "        vertexUV.x *= (t.x * i.y) / (t.y * i.x);            \n"
	"    }                                                       \n"
	"    else {                                                  \n"
    "        vertexUV.y -= (t.y-t.x/i.x*i.y)/(2.0*t.y);          \n"
    "        vertexUV.y *= (t.y * i.x) / (t.x * i.y);            \n"
	"    }                                                       \n"
	"}                                                           \n";

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

uint fileIndex = 0;

static void load_textures() {
	DIR *d;
	struct dirent *dir;
	
	const char* pics = "/home/pi/Projects/Pictures/%s";

	fileIndex++;

	while (true) {
		d = opendir("/home/pi/Projects/Pictures");
		int j = 0;
		while ((dir = readdir(d)) != NULL) {
			if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
			if (j == fileIndex) {
				char* file;
				asprintf(&file, pics, dir->d_name);
				printf("Loading %s\n", file);
				unsigned int x, y, n;
				unsigned char* data = stbi_load(file, &x, &y, &n, 4);
				printf("%d x %d with %d channels\n", x, y, n);

				glBindTexture(GL_TEXTURE_2D, tex1);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
				glGenerateMipmap(GL_TEXTURE_2D);
				glUniform2f(i, x, y);
				
				free(file);
				free(data);

				if (glGetError() != 0) {
					printf("Error loading texture, trying next\n");

					continue;
				}
				closedir(d);
				return;
			}
			j++;
		}
		fileIndex = 0;
		closedir(d);
	}
}

uint64_t picture_start = 0;

int switchTime = 10;

static void draw_shadertoy() {
	if (picture_start == 0) {
		picture_start = get_time_ns();
	}

	double time = (get_time_ns() - picture_start) / (double) NSEC_PER_SEC;

	if (time > switchTime) {
		load_textures();
		picture_start = get_time_ns();
		time = (get_time_ns() - picture_start) / (double) NSEC_PER_SEC;
	}
	glUniform1f(iBlend, time / 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(vao);
	
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

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex1);

	glUseProgram(program);

	iBlend = glGetUniformLocation(program, "iBlend");
	i = glGetUniformLocation(program, "i");
	iTex1 = glGetUniformLocation(program, "texture1");
	iTex2 = glGetUniformLocation(program, "texture2");
	GLint t = glGetUniformLocation(program, "t");
	// glUniform2f(t, gbm->width, gbm->height);
	glUniform2f(t, 1440, 1080); // Since my screen is 4:3 physically, need to pretend it is in resolution
	printf("Target resolution: %d x %d\n", gbm->width, gbm->height);

	tx = gbm->width;
	ty = gbm->height;

	glGenTextures(1, tex1);
	glActiveTexture(GL_TEXTURE0);
	load_textures();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_NV);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_NV);

	float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_NV, borderColor);

	glUniform1i(iTex1, 0);
	glUniform1i(iTex2, 1);

	egl->draw = draw_shadertoy;

	return 0;
}
