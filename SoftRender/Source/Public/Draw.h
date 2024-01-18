#ifndef DRAW_H
#define DRAW_H

#include "FrameBuffer.h"
#include "Shader.h"
#include "Model.h"
#include <random>
//#include "Model.h"
//#include "Camera.h"
//#include "Clip.h"
//#include "Cull.h"
//#include "PBR.h"
//#include "Skybox.h"

using namespace glm;
using namespace cv;

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


struct GouraudShader : public IShader {
	Model& model;
	mat4 transMatrix;
	vec3 light_dir;
	vec3 varying_intensity; // written by vertex shader, read by fragment shader
	vec3 varying_color[3];

	GouraudShader(Model& m, vec3 light, mat4 tm) : model(m), light_dir(light), transMatrix(tm) {
		model = m;
	}

	virtual vec4 vertex(int iface, int nthvert) {
		vec4 gl_Vertex = embed(model.vert(iface, nthvert)); // read the vertex from .obj file
		//gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;     // transform it to screen coordinates
		float cur_intensity = -(dot(model.normal(iface, nthvert), light_dir));
		varying_intensity[nthvert] = std::max(0.f, cur_intensity); // get diffuse lighting intensity
		//return transMatrix * gl_Vertex;
		return transMatrix * gl_Vertex;
	}

	virtual bool fragment(vec3 bar, vec4& color) {
		float intensity = dot(varying_intensity, bar);   // interpolate intensity for the current pixel
		//if (intensity <= 0.f) return true;
		color = vec4(255, 255, 255, 255) * intensity; // well duh
		return false;                              // no, we do not discard this pixel
	}
};

class Draw {

private:
public:
	int Width;
	int Height;
	FrameBuffer* FrontBuffer;
	Shader* shader;
	glm::mat4 ViewPortMatrix;
public:
	Draw(const int& w, const int& h) :
		Width(w), Height(h), FrontBuffer(nullptr), shader(nullptr) {}
	~Draw() {
		if (FrontBuffer)
			delete FrontBuffer;
		if (shader)
			delete shader;
		FrontBuffer = nullptr;
		shader = nullptr;
	}
	/*void setModelMatrix(const glm::mat4& model) {
		shader->setModelMatrix(model);
	}
	void setViewMatrix(const glm::mat4& view) {
		shader->setViewMatrix(view);
	}
	void setProjectMatrix(const glm::mat4& project) {
		shader->setProjectMatrix(project);
	}*/
	void Init() {
		if (FrontBuffer)
			delete FrontBuffer;
		if (shader)
			delete shader;
		ViewPortMatrix = GetViewPortMatrix(0, 0, Width, Height);
		FrontBuffer = new FrameBuffer(Width, Height);
		shader = new Shader();
	}
	void Resize(const int& w, const int& h) {
		Width = w;
		Height = h;
		FrontBuffer->Resize(w, h);
		ViewPortMatrix = GetViewPortMatrix(0, 0, w, h);
	}
	void ClearBuffer(const glm::vec4& color) {
		FrontBuffer->ClearColorBuffer(color);
	}
	void Show() {
		glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, FrontBuffer->colorBuffer.data());
	}

	vec3 light_dir = normalize(vec3(1, -1, 1));
	vec3 eye = vec3(1, 1, 3);
	vec3 center = vec3(0, 0, 0);
	vec3 up{ 0,1,0 }; // camera up vector


	mat4 ModelView;
	mat4 Viewport;
	mat4 Projection;


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


	void drawModel(const char* filename) {
		//Model* model = new Model("obj/african_head.obj");
		Model* model = new Model("obj/diablo3_pose.obj");

		//Shader shader;
		lookat(eye, center, up);                            // build the ModelView matrix
		viewport(Width / 8, Height / 8, Width * 3 / 4, Height * 3 / 4); // build the Viewport matrix
		projection(-1.f / length(eye - center));                    // build the Projection matrix
		GouraudShader shader(*model, light_dir, Viewport);
		FrameBuffer* shaderFrame = new FrameBuffer(Width, Height);

		glm::vec3 light_dir(0, 0, -1);
		float* zbuffer = new float[Width * Height];
		float* zbuffer2 = new float[Width * Height];
		for (int i = Width * Height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			glm::vec3 world_pts[3];
			glm::vec3 pts[3];
			glm::vec4 clip_vert[3];
			for (int j = 0; j < 3; j++) {
				world_pts[j] = model->vert(face[j]);
				pts[j] = world2screen(world_pts[j]);
				clip_vert[j] = shader.vertex(i, j);
			}
			glm::vec3 triangle_normal = glm::cross((world_pts[2] - world_pts[0]), (world_pts[1] - world_pts[0]));
			float intensity = glm::dot(glm::normalize(triangle_normal), light_dir);
			if (intensity <= 0)continue;
			glm::vec2 uv[3];
			for (int k = 0; k < 3; k++) {
				uv[k] = model->uv(i, k);
			}
			triangle(model, pts, clip_vert, shader, zbuffer, zbuffer2, *shaderFrame, uv, glm::vec4(intensity, intensity, intensity, intensity));

			triangle2(model, clip_vert, uv, shader, zbuffer2, *shaderFrame);

			//randnom color
			//std::default_random_engine e;
			//std::uniform_int_distribution<int> u(0, 255); // ����ұ�����
			//e.seed(i);
			//triangle(pts, zbuffer, glm::vec4(u(e), u(e), u(e), u(e)));

		}


		cv::Mat shaderResult = Mat(Height, Width, CV_8UC4, shaderFrame->colorBuffer.data()).clone();
		flip(shaderResult, shaderResult, 0);
		cv::imshow("shaderResult", shaderResult);
		delete shaderFrame;
		delete model;

		delete[]zbuffer;
		delete[]zbuffer2;
	}

	glm::vec3 world2screen(glm::vec3  v) {
		return glm::vec3(int((v.x + 1.) * Width / 2. + .5), int((v.y + 1.) * Height / 2. + .5), v.z);
	}

	glm::vec3 barycentric(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 P) {
		glm::vec3 s[2];
		for (int i = 2; i--; ) {
			s[i][0] = C[i] - A[i];
			s[i][1] = B[i] - A[i];
			s[i][2] = A[i] - P[i];
		}
		glm::vec3 u = cross(s[0], s[1]);
		if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
			return glm::vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
		return glm::vec3(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
	}

	void triangle(Model* model, glm::vec3* pts, glm::vec4* clip_vert, IShader& shader, float* zbuffer, float* zbuffer2, FrameBuffer& shaderResult, glm::vec2* uv, glm::vec4 intensity) {
		glm::vec2 bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		glm::vec2 bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
		glm::vec2 clamp(Width - 1, Height - 1);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 2; j++) {
				bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
				bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
			}
		}
		glm::vec3 P;
		for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
			for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
				glm::vec3 c = barycentric(pts[0], pts[1], pts[2], P);
				glm::vec3 uv_screen = barycentric(glm::vec3(uv[0], 0), glm::vec3(uv[1], 0), glm::vec3(uv[2], 0), P);

				if (c.x < 0 || c.y < 0 || c.z < 0) continue;
				P.z = 0;
				glm::vec2 uvP(0, 0);
				for (int i = 0; i < 3; i++) {
					P.z += pts[i][2] * c[i];
					uvP.x += uv[i][0] * c[i];
					uvP.y += uv[i][1] * c[i];
				}
				//uvP = uv[0];
				if (zbuffer[int(P.x + P.y * Width)] < P.z) {
					zbuffer[int(P.x + P.y * Width)] = P.z;
					glm::vec4 color = model->diffuse(uvP);
					FrontBuffer->WritePoint(P.x, P.y, color * intensity);

					vec4 light_color;
					bool discard = shader.fragment(c, light_color);
					//shaderResult.WritePoint(P.x, P.y, light_color);
					//shaderResult.WritePoint(P.x, P.y, color * light_color / 255.f);
				}
			}
		}
	}

	void triangle2(Model* model, glm::vec4* clip_vert, glm::vec2* uv, IShader& shader, float* zbuffer, FrameBuffer& shaderResult) {
		glm::vec2 bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		glm::vec2 bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 2; j++) {
				bboxmin[j] = std::min(bboxmin[j], clip_vert[i][j] / clip_vert[i][3]);
				bboxmax[j] = std::max(bboxmax[j], clip_vert[i][j] / clip_vert[i][3]);
			}
		}
		glm::vec3 P(0,0,0);
		for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
			for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
				glm::vec4 bc_vert[4];
				for (size_t i = 0; i < 3; i++)
				{
					bc_vert[i] = clip_vert[i] / clip_vert[i][3];
					bc_vert[i][2] = 0;
				}

				glm::vec3 c = barycentric(bc_vert[0], bc_vert[1], bc_vert[2], P);
				if (c.x < 0 || c.y < 0 || c.z < 0) continue;
				float z = clip_vert[0][2] * c.x + clip_vert[1][2] * c.y + clip_vert[2][2] * c.z;
				float w = clip_vert[0][3] * c.x + clip_vert[1][3] * c.y + clip_vert[2][3] * c.z;

				/*glm::vec4 diffuseColor(0, 0, 0, 255);
				for (int i = 0; i < 3; i++) {
					diffuseColor.x += model->diffuse(uv[i])[0] * c[i];
					diffuseColor.y += model->diffuse(uv[i])[1] * c[i];
					diffuseColor.z += model->diffuse(uv[i])[1] * c[i];
				}*/
				if (zbuffer[int(P.x + P.y * Width)] < z / w) {
					zbuffer[int(P.x + P.y * Width)] = z / w;
					vec4 color;
					bool discard = shader.fragment(c, color);
					//if (!discard) {
					//	//zbuffer[P.x + P.y * image.Width] = frag_depth;
					//	shaderResult.WritePoint(P.x, P.y, fcolor);
					//}
					//shaderResult.WritePoint(P.x, P.y, diffuseColor);
					shaderResult.WritePoint(P.x, P.y, color);
				}
			}
		}
	}
};
#endif