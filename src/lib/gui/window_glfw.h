/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "gl_2d_display.h"
#include "mat_to_gl.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace cimbar {

class window_glfw
{
public:
	window_glfw(unsigned width, unsigned height, std::string title)
	    : _width(width)
	{
		if (!glfwInit())
		{
			_good = false;
			return;
		}

		// Add DPI awareness disable BEFORE window creation
		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
		glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);

		_w = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		if (!_w)
		{
			_good = false;
			return;
		}
		glfwMakeContextCurrent(_w);
		glfwSwapInterval(1);

		_display = std::make_shared<cimbar::gl_2d_display>(width, height);
		glGenTextures(1, &_texid);
		init_opengl(width, height);
	}

	~window_glfw()
	{
		if (_w)
			glfwDestroyWindow(_w);
		if (_texid)
			glDeleteTextures(1, &_texid);
		glfwTerminate();
	}

	bool is_good() const
	{
		return _good;
	}

	bool is_minimized() const
	{
		return glfwGetWindowAttrib(_w, GLFW_ICONIFIED);
	}

	bool should_close() const
	{
		return glfwWindowShouldClose(_w);
	}

	void auto_scale_to_window()
	{
		if (!is_good())
			return;
		auto fun = [](GLFWwindow*, int w, int h){ glViewport(0, 0, w, h); };
		glfwSetWindowSizeCallback(_w, fun);
	}

	void rotate(unsigned i=1)
	{
		if (_display)
			_display->rotate(i);
	}

	void shake(unsigned i=1)
	{
		if (_display)
			_display->shake(i);
	}

	void clear()
	{
		if (_display)
		{
			_display->clear();
			swap();
		}
	}

	void show(const cv::Mat& img, unsigned delay)
	{
		std::chrono::time_point start = std::chrono::high_resolution_clock::now();

		if (_display)
		{
			cimbar::mat_to_gl::load_gl_texture(_texid, img);
			_display->draw(_texid);

			swap();
			poll();
		}

		unsigned millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
		if (delay > millis)
			std::this_thread::sleep_for(std::chrono::milliseconds(delay-millis));
	}

	unsigned width() const
	{
		return _width;
	}

	void swap()
	{
		// show next frame
		glfwSwapBuffers(_w);
	}

	void poll()
	{
		glfwPollEvents();
	}

	void set_title(std::string title)
	{
		glfwSetWindowTitle(_w, title.c_str());
	}

	void set_mouse_callback(GLFWcursorposfun fun)
	{
		glfwSetCursorPosCallback(_w, fun);
	}

	void set_mouse_button_callback(GLFWmousebuttonfun fun)
	{
		glfwSetMouseButtonCallback(_w, fun);
	}

	void set_key_callback(GLFWkeyfun fun)
	{
		glfwSetKeyCallback(_w, fun);
	}

	void set_scroll_callback(GLFWscrollfun fun)
	{
		glfwSetScrollCallback(_w, fun);
	}

protected:
	void init_opengl(int width, int height)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, width, height);
	}

protected:
	GLFWwindow* _w;
	GLuint _texid;
	std::shared_ptr<cimbar::gl_2d_display> _display;
	unsigned _width;
	bool _good = true;
};

}
