
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Math.h"
#include <glm/glm.hpp>
#include <vector>
class FrameBuffer {
public:
	int Width, Height;
	std::vector<unsigned char> colorBuffer;
	~FrameBuffer() = default;
	FrameBuffer(const int& w = 800, const int& h = 600) {
		Width = w;
		Height = h;
		//RGBA
		colorBuffer.resize(w * h * 4, 0);
	}
	void Resize(const int& w, const int& h) {
		Width = w;
		Height = h;
		colorBuffer.resize(w * h * 4, 0);
	}
	void ClearColorBuffer(const glm::vec4& color) {
		unsigned char* p = colorBuffer.data();
		for (int i = 0; i < Width * Height * 4; i += 4) {
			*(p + i) = color.r;
			*(p + i + 1) = color.g;
			*(p + i + 2) = color.b;
			*(p + i + 3) = color.a;
		}
		/*
		int length = Width * Height * 4;
		for (int i = 0; i < length; i+=4) {
			colorBuffer[i] = 255;
			colorBuffer[i + 1] = 0;
			colorBuffer[i + 2] = 0;
			colorBuffer[i + 3] = 0;
		}*/
	}
	void WritePoint(const int& x, const int& y, const glm::vec4& color) {
		if (x < 0 || x >= Width || y < 0 || y >= Height)
			return;
		int xy = (y * Width + x);
		unsigned char* p = colorBuffer.data();
		*(p + xy * 4) = color.r;
		*(p + xy * 4 + 1) = color.g;
		*(p + xy * 4 + 2) = color.b;
		*(p + xy * 4 + 3) = color.a;
	}
};

#endif