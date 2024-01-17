#include <vector>
#include "FrameBuffer.h"
#include <glm/glm.hpp>
#include<opencv2/opencv.hpp>
using namespace glm;
using namespace cv;

void viewport(const int x, const int y, const int w, const int h);
void projection(const double coeff = 0); // coeff = -1/c
void lookat(const vec3 eye, const vec3 center, const vec3 up);

static vec4 embed(vec3 v, double fill = 1) {
	return vec4(v, fill);
}
static vec3 embed(vec2 v, double fill = 1) {
	return vec3(v, fill);
}
static vec3 proj(vec4 v) {
	return vec3(v);
}
static vec2 proj(vec3 v) {
	return vec2(v);
}
static vec1 proj(vec2 v) {
	return vec1(v);
}


struct IShader {
	static vec4 sample2D(const Mat& img, vec2& uvf) {
		//return img.get(uvf[0] * img.width(), uvf[1] * img.height());
		Vec3b cvColor = img.at<Vec3b>(int(img.cols - uvf.y), int(uvf.x));

		return vec4(cvColor[0], cvColor[1], cvColor[2], 1);
	}
	virtual bool fragment(const vec3 bar, vec4& color) = 0;
};

void viewport(const int x, const int y, const int w, const int h);

void triangle(const vec4 clip_verts[3], IShader& shader, FrameBuffer image, std::vector<double>& zbuffer);

mat4 ModelView;
mat4 Viewport;
mat4 Projection;

//template<int n1, int n2> vec<n1> embed(const vec<n2>& v, double fill = 1) {
//	vec<n1> ret;
//	for (int i = n1; i--; ret[i] = (i < n2 ? v[i] : fill));
//	return ret;
//}
//
//template<int n1, int n2> vec<n1> proj(const vec<n2>& v) {
//	vec<n1> ret;
//	for (int i = n1; i--; ret[i] = v[i]);
//	return ret;
//}

void viewport(const int x, const int y, const int w, const int h) {
	Viewport = mat4(1.0f);
	Viewport[0][3] = x + w / 2.f;
	Viewport[1][3] = y + h / 2.f;
	Viewport[2][3] = 255.f / 2.f;
	Viewport[0][0] = w / 2.f;
	Viewport[1][1] = h / 2.f;
	Viewport[2][2] = 255.f / 2.f;
	Viewport = transpose(Viewport);
}

void projection(float coeff) { // check https://en.wikipedia.org/wiki/Camera_matrix
	Projection = mat4(1.0f);
	Projection[3][2] = coeff;
	Projection = transpose(Projection);
}

void lookat(const vec3 eye, const vec3 center, const vec3 up) { // check https://github.com/ssloy/tinyrenderer/wiki/Lesson-5-Moving-the-camera
	vec3 z = normalize(center - eye);
	vec3 x = normalize(cross(up, z));
	vec3 y = normalize(cross(z, x));
	ModelView = mat4(1.0f);
	for (int i = 0; i < 3; i++) {
		ModelView[0][i] = x[i];
		ModelView[1][i] = y[i];
		ModelView[2][i] = z[i];
		ModelView[i][3] = -center[i];
	}
	Projection = transpose(Projection);
}

vec3 barycentric(vec2 A, vec2 B, vec2 C, vec2 P) {
	vec3 s[2];
	for (int i = 2; i--; ) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	vec3 u = cross(s[0], s[1]);
	if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return vec3(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(vec4 pts[3], IShader& shader, FrameBuffer& image, std::vector<double>& zbuffer) {
	vec2 bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	vec2 bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::min(bboxmin[j], pts[i][j] / pts[i][3]);
			bboxmax[j] = std::max(bboxmax[j], pts[i][j] / pts[i][3]);
		}
	}
	vec2 P;
	vec4 color;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			vec3 c = barycentric(proj(pts[0] / pts[0][3]), proj(pts[1] / pts[1][3]), proj(pts[2] / pts[2][3]), (P));
			float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
			float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
			int frag_depth = std::max(0, std::min(255, int(z / w + .5)));
			//float frag_depth = std::max(0.f, (z / w));
			//if (c.x < 0 || c.y < 0 || c.z<0 || zbuffer[P.x + P.y * image.Width]>frag_depth) continue;
			if (c.x < 0) continue;
			if (c.y < 0) continue;
			if (c.z < 0) continue;
			if (zbuffer[P.x + P.y * image.Width] > frag_depth) continue;
			bool discard = shader.fragment(c, color);
			if (!discard) {
				zbuffer[P.x + P.y * image.Width] = frag_depth;
				image.WritePoint(P.x, P.y, color);
			}
		}
	}
}