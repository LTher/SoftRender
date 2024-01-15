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
//static vec3 proj(vec4 v) {
//	return vec3(v);
//}


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
	Viewport = mat4(vec4(w / 2., 0, 0, x + w / 2.), vec4(0, h / 2., 0, y + h / 2.), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1));
}

void projection(const double f) { // check https://en.wikipedia.org/wiki/Camera_matrix
	Projection = mat4(vec4(1, 0, 0, 0), vec4(0, -1, 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, -1 / f, 0));
}

void lookat(const vec3 eye, const vec3 center, const vec3 up) { // check https://github.com/ssloy/tinyrenderer/wiki/Lesson-5-Moving-the-camera
	vec3 z = normalize(center - eye);
	vec3 x = normalize(cross(up, z));
	vec3 y = normalize(cross(z, x));
	mat4 Minv = mat4(vec4(x.x, x.y, x.z, 0), vec4(y.x, y.y, y.z, 0), vec4(z.x, z.y, z.z, 0), vec4(0, 0, 0, 1));
	mat4 Tr = mat4(vec4(1, 0, 0, -eye.x), vec4(0, 1, 0, -eye.y), vec4(0, 0, 1, -eye.z), vec4(0, 0, 0, 1));
	ModelView = Minv * Tr;
}

vec3 barycentric(const vec2 tri[3], const vec2 P) {
	mat3 ABC = { {embed(tri[0]), embed(tri[1]), embed(tri[2])} };
	if (glm::determinant(ABC) < 1e-3) return { -1,1,1 }; // for a degenerate triangle generate negative coordinates, it will be thrown away by the rasterizator
	//return ABC.invert_transpose() * embed<3>(P);
	return glm::inverse(ABC) * embed(P);
}

void triangle(const vec4 clip_verts[3], IShader& shader, FrameBuffer image, std::vector<double>& zbuffer) {
	vec4 pts[3] = { Viewport * clip_verts[0],    Viewport * clip_verts[1],    Viewport * clip_verts[2] };  // triangle screen coordinates before persp. division
	vec2 pts2[3] = { vec2(pts[0] / pts[0][3]), vec2(pts[1] / pts[1][3]), vec2(pts[2] / pts[2][3]) };  // triangle screen coordinates after  perps. division

	int bboxmin[2] = { image.Width - 1, image.Height - 1 };
	int bboxmax[2] = { 0, 0 };
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::min(bboxmin[j], static_cast<int>(pts2[i][j]));
			bboxmax[j] = std::max(bboxmax[j], static_cast<int>(pts2[i][j]));
		}
#pragma omp parallel for
	for (int x = std::max(bboxmin[0], 0); x <= std::min(bboxmax[0], image.Width - 1); x++) {
		for (int y = std::max(bboxmin[1], 0); y <= std::min(bboxmax[1], image.Height - 1); y++) {
			vec3 bc_screen = barycentric(pts2, { static_cast<double>(x), static_cast<double>(y) });
			vec3 bc_clip = { bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3] };
			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z); // check https://github.com/ssloy/tinyrenderer/wiki/Technical-difficulties-linear-interpolation-with-perspective-deformations
			double frag_depth = glm::dot(vec3(clip_verts[0][2], clip_verts[1][2], clip_verts[2][2]), bc_clip);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z<0 || frag_depth > zbuffer[x + y * image.Width]) continue;
			vec4 color;
			if (shader.fragment(bc_clip, color)) continue; // fragment shader can discard current fragment
			zbuffer[x + y * image.Width] = frag_depth;
			image.WritePoint(x, y, color);
		}
	}
}