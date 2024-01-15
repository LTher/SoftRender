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
class Draw {

private:
public:
	int Width;
	int Height;
	int Depth = 255;
	glm::vec3 Light_dir = glm::vec3(0, 0, -1);
	glm::vec3 Camera = glm::vec3(0, 0, 3);
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

	Model* loadModel(const char* filename) {
		return new Model(filename);
	}

	void drawModel_old(Model* model) {
		//Model* model = new Model("obj/african_head.obj");
		//Model* model = new Model("obj/diablo3_pose.obj");

		Shader shader;

		// draw lines
		/*for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			for (int j = 0; j < 3; j++) {
				glm::vec3 v0 = model->vert(face[j]);
				glm::vec3 v1 = model->vert(face[(j + 1) % 3]);

				Vertex V1(v0, glm::vec4(255, 0, 0, 0));
				Vertex V2(v1);

				V2F o1 = shader.VertexShader(V1);
				V2F o2 = shader.VertexShader(V2);

				o1.windowPos = ViewPortMatrix * o1.windowPos;
				o2.windowPos = ViewPortMatrix * o2.windowPos;
				DrawLine(o1, o2);
			}
		}*/

		//glm::vec3 light_dir(0, 0, -1);
		//for (int i = 0; i < model->nfaces(); i++) {
		//	std::vector<int> face = model->face(i);
		//	glm::vec3 world_coords[3];
		//	V2F v2fP[3];
		//	for (int j = 0; j < 3; j++) {
		//		world_coords[j] = model->vert(face[j]);
		//		
		//	}/*
		//	triangle(screen_coords[0], screen_coords[1], screen_coords[2], image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));*/

		//	glm::vec3 triangle_normal = glm::cross((world_coords[2] - world_coords[0]),(world_coords[1] - world_coords[0]));
		//	float intensity = glm::dot(glm::normalize(triangle_normal) , light_dir) * 255;
		//	if (intensity <= 0)continue;
		//	for (int j = 0; j < 3; j++) {
		//		//randnom color
		//		//std::default_random_engine e;
		//		//std::uniform_int_distribution<int> u(0, 255); // 左闭右闭区间
		//		//e.seed(i);
		//		//Vertex Vtemp(world_coords[j], glm::vec4(u(e), u(e), u(e), u(e)));

		//		Vertex Vtemp(world_coords[j], glm::vec4(intensity, intensity, intensity, intensity));
		//		v2fP[j] = shader.VertexShader(Vtemp);
		//		v2fP[j].windowPos = ViewPortMatrix * v2fP[j].windowPos;
		//	}

		//	ScanLineTriangle(v2fP[0], v2fP[1], v2fP[2]);
		//}

		glm::vec3 light_dir(0, 0, -1);
		float* zbuffer = new float[Width * Height];
		for (int i = Width * Height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			glm::vec3 world_pts[3];
			glm::vec3 pts[3];
			for (int i = 0; i < 3; i++) {
				world_pts[i] = model->vert(face[i]);
				pts[i] = world2screen(world_pts[i]);
			}
			glm::vec3 triangle_normal = glm::cross((world_pts[2] - world_pts[0]), (world_pts[1] - world_pts[0]));
			float intensity = glm::dot(glm::normalize(triangle_normal), light_dir);
			if (intensity <= 0)continue;
			glm::vec2 uv[3];
			glm::vec3 normal[3];
			for (int k = 0; k < 3; k++) {
				uv[k] = model->uv(i, k);
				normal[k] = model->normal(i, k);
			}
			//triangle(model, zbuffer, pts, uv, normal, light_dir, glm::vec4(intensity, intensity, intensity, intensity));

			//random color
			//std::default_random_engine e;
			//std::uniform_int_distribution<int> u(0, 255); // 左闭右闭区间
			//e.seed(i);
			//triangle(pts, zbuffer, glm::vec4(u(e), u(e), u(e), u(e)));

		}
		delete[] zbuffer;
		//delete model;
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

	void triangle_old(Model* model, float* zbuffer, glm::vec3* pts, glm::vec2* uvs, glm::vec3* normals, glm::vec3 light_dir, glm::vec4 triangle_light_intensity) {
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
				glm::vec3 bc_screen = barycentric(pts[0], pts[1], pts[2], P);
				glm::vec3 uv_screen = barycentric(glm::vec3(uvs[0], 0), glm::vec3(uvs[1], 0), glm::vec3(uvs[2], 0), P);
				if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
				P.z = 0;
				glm::vec2 uvP(0, 0);
				glm::vec3 normalP(0, 0, 0);
				for (int i = 0; i < 3; i++) {
					P.z += pts[i][2] * bc_screen[i];
					uvP.x += uvs[i][0] * bc_screen[i];
					uvP.y += uvs[i][1] * bc_screen[i];
					normalP.x += normals[i][0] * bc_screen[i];
					normalP.y += normals[i][1] * bc_screen[i];
					normalP.z += normals[i][2] * bc_screen[i];
				}
				//uvP = uv[0];
				if (zbuffer[int(P.x + P.y * Width)] < P.z) {
					zbuffer[int(P.x + P.y * Height)] = P.z;
					glm::vec4 color = model->diffuse(uvP);
					float intensity = glm::dot(glm::normalize(normalP), light_dir);
					//if (intensity <= 0)continue;
					glm::vec4 light = glm::vec4(intensity, intensity, intensity, intensity);
					FrontBuffer->WritePoint(P.x, P.y, color * triangle_light_intensity);
				}
			}
		}
	}

	glm::vec3 m2v(glm::mat4 m) {
		return glm::vec3(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
	}

	glm::mat4 v2m(glm::vec3 v) {
		glm::mat4 m(1.0f);
		m[0][0] = v.x;
		m[1][0] = v.y;
		m[2][0] = v.z;
		m[3][0] = 1.f;
		return m;
	}

	glm::mat4 viewport(int x, int y, int w, int h) {
		glm::mat4  m(1.0f);
		m[0][3] = x + w / 2.f;
		m[1][3] = y + h / 2.f;
		m[2][3] = Depth / 2.f;

		m[0][0] = w / 2.f;
		m[1][1] = h / 2.f;
		m[2][2] = Depth / 2.f;
		return m;
	}

	void drawModel(Model* model) {
		int* zbuffer = new int[Width * Height];
		glm::mat4 Projection(1.0f);
		glm::mat4 ViewPort = viewport(Width / 8, Height / 8, Width * 3 / 4, Height * 3 / 4);
		Projection[3][2] = -1.f / Camera.z;

		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			glm::vec3 screen_coords[3];
			glm::vec3 world_coords[3];
			for (int j = 0; j < 3; j++) {
				glm::vec3 v = model->vert(face[j]);
				glm::vec4 mv = glm::vec4((v + 1.f) / 2.f, 1.0f);
				glm::vec4 fv = ViewPort * Projection * mv;
				screen_coords[j] = fv ;
				world_coords[j] = v;
			}
			glm::vec3 n = glm::cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
			float intensity = glm::dot(glm::normalize(n), Light_dir);
			if (intensity > 0) {
				glm::vec2 uv[3];
				for (int k = 0; k < 3; k++) {
					uv[k] = model->uv(i, k);
				}
				triangle(model, screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2], intensity, zbuffer);
			}
		}
		delete[] zbuffer;
	}

	//void triangle(Model* model, glm::i32vec3 t0, glm::i32vec3 t1, glm::i32vec3 t2, glm::i32vec2 uv0, glm::i32vec2 uv1, glm::i32vec2 uv2, float intensity, int* zbuffer) {
	//	if (t0.y == t1.y && t0.y == t2.y) return; // i dont care about degenerate triangles
	//	if (t0.y > t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
	//	if (t0.y > t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
	//	if (t1.y > t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

	void triangle(Model* model, glm::vec3 t0, glm::vec3 t1, glm::vec3 t2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, float intensity, int* zbuffer) {
		if (t0.y == t1.y && t0.y == t2.y) return; // i dont care about degenerate triangles
		if (t0.y > t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
		if (t0.y > t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
		if (t1.y > t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

		int total_height = t2.y - t0.y;
		for (int i = 0; i < total_height; i++) {
			bool second_half = i > t1.y - t0.y || t1.y == t0.y;
			int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
			float alpha = (float)i / total_height;
			float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
			glm::vec3 A = t0 + glm::vec3(glm::vec3(t2 - t0) * alpha);
			glm::vec3 B = second_half ? t1 + glm::vec3(glm::vec3(t2 - t1) * beta) : t0 + glm::vec3(glm::vec3(t1 - t0) * beta);
			glm::vec2 uvA = uv0 + glm::vec2(glm::vec2(uv2 - uv0) * alpha);
			glm::vec2 uvB = second_half ? uv1 + glm::vec2(glm::vec2(uv2 - uv1) * beta) : uv0 + glm::vec2(glm::vec2(uv1 - uv0) * beta);
			if (A.x > B.x) { std::swap(A, B); std::swap(uvA, uvB); }
			for (int j = A.x; j <= B.x; j++) {
				float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
				if (phi < 0)phi = 0;
				glm::vec3  P = glm::vec3(A) + glm::vec3(B - A) * phi;
				glm::vec2 uvP = uvA + glm::vec2(glm::vec2(uvB - uvA) * phi);
				int idx = (P.x) + (P.y) * Width;
				if (zbuffer[idx] < P.z) {
					zbuffer[idx] = P.z;
					/*TGAColor color = model->diffuse(uvP);
					image.set(P.x, P.y, TGAColor(color.r * intensity, color.g * intensity, color.b * intensity));*/
					glm::vec4 color = model->diffuse(uvP);
					FrontBuffer->WritePoint(P.x, P.y, color * intensity);
				}
			}
		}
	}

	//	int total_height = t2.y - t0.y;
	//	for (int i = 0; i < total_height; i++) {
	//		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
	//		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
	//		float alpha = (float)i / total_height;
	//		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
	//		glm::i32vec3 A = t0 + glm::i32vec3(glm::f32vec3(t2 - t0) * alpha);
	//		glm::i32vec3 B = second_half ? t1 + glm::i32vec3(glm::f32vec3(t2 - t1) * beta) : t0 + glm::i32vec3(glm::f32vec3(t1 - t0) * beta);
	//		glm::i32vec2 uvA = uv0 + glm::i32vec2(glm::f32vec2(uv2 - uv0) * alpha);
	//		glm::i32vec2 uvB = second_half ? uv1 + glm::i32vec2(glm::f32vec2(uv2 - uv1) * beta) : uv0 + glm::i32vec2(glm::f32vec2(uv1 - uv0) * beta);
	//		if (A.x > B.x) { std::swap(A, B); std::swap(uvA, uvB); }
	//		for (int j = A.x; j <= B.x; j++) {
	//			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
	//			glm::i32vec3  P = glm::f32vec3(A) + glm::f32vec3(B - A) * phi;
	//			glm::i32vec2 uvP = uvA + glm::i32vec2(glm::f32vec2(uvB - uvA) * phi);
	//			int idx = (P.x+1.f) + (P.y + 1.f)* Width;
	//			if (zbuffer[idx] < P.z) {
	//				zbuffer[idx] = P.z;
	//				/*TGAColor color = model->diffuse(uvP);
	//				image.set(P.x, P.y, TGAColor(color.r * intensity, color.g * intensity, color.b * intensity));*/
	//				glm::vec4 color = model->diffuse(uvP);
	//				FrontBuffer->WritePoint(P.x, P.y, color * intensity);
	//			}
	//		}
	//	}
	//}

#pragma region Rasterization
	//扫描线填充算法
	//对任意三角形，分为上下两个平底三角形填充
	void ScanLineTriangle(const V2F& v1, const V2F& v2, const V2F& v3) {

		std::vector<V2F> arr = { v1,v2,v3 };
		if (arr[0].windowPos.y > arr[1].windowPos.y) {
			V2F tmp = arr[0];
			arr[0] = arr[1];
			arr[1] = tmp;
		}
		if (arr[1].windowPos.y > arr[2].windowPos.y) {
			V2F tmp = arr[1];
			arr[1] = arr[2];
			arr[2] = tmp;
		}
		if (arr[0].windowPos.y > arr[1].windowPos.y) {
			V2F tmp = arr[0];
			arr[0] = arr[1];
			arr[1] = tmp;
		}
		//arr[0] 在最下面  arr[2]在最上面

		//中间跟上面的相等，是底三角形
		if (equal(arr[1].windowPos.y, arr[2].windowPos.y)) {
			DownTriangle(arr[1], arr[2], arr[0]);
		}//顶三角形
		else if (equal(arr[1].windowPos.y, arr[0].windowPos.y)) {
			UpTriangle(arr[1], arr[0], arr[2]);
		}
		else {
			float weight = (arr[2].windowPos.y - arr[1].windowPos.y) / (arr[2].windowPos.y - arr[0].windowPos.y);
			V2F newEdge = V2F::lerp(arr[2], arr[0], weight);
			UpTriangle(arr[1], newEdge, arr[2]);
			DownTriangle(arr[1], newEdge, arr[0]);
		}


	}
	void UpTriangle(const V2F& v1, const V2F& v2, const V2F& v3) {
		V2F left, right, top;
		left = v1.windowPos.x > v2.windowPos.x ? v2 : v1;
		right = v1.windowPos.x > v2.windowPos.x ? v1 : v2;
		top = v3;
		left.windowPos.x = int(left.windowPos.x);
		int dy = top.windowPos.y - left.windowPos.y;
		int nowY = top.windowPos.y;
		//从上往下插值
		for (int i = dy; i >= 0; i--) {
			float weight = 0;
			if (dy != 0) {
				weight = (float)i / dy;
			}
			V2F newLeft = V2F::lerp(left, top, weight);
			V2F newRight = V2F::lerp(right, top, weight);
			newLeft.windowPos.x = int(newLeft.windowPos.x);
			newRight.windowPos.x = int(newRight.windowPos.x + 0.5);
			newLeft.windowPos.y = newRight.windowPos.y = nowY;
			ScanLine(newLeft, newRight);
			nowY--;
		}
	}
	void DownTriangle(const V2F& v1, const V2F& v2, const V2F& v3) {
		V2F left, right, bottom;
		left = v1.windowPos.x > v2.windowPos.x ? v2 : v1;
		right = v1.windowPos.x > v2.windowPos.x ? v1 : v2;
		bottom = v3;

		int dy = left.windowPos.y - bottom.windowPos.y;
		int nowY = left.windowPos.y;
		//从上往下插值
		for (int i = 0; i < dy; i++) {
			float weight = 0;
			if (dy != 0) {
				weight = (float)i / dy;
			}
			V2F newLeft = V2F::lerp(left, bottom, weight);
			V2F newRight = V2F::lerp(right, bottom, weight);
			newLeft.windowPos.x = int(newLeft.windowPos.x);
			newRight.windowPos.x = int(newRight.windowPos.x + 0.5);
			newLeft.windowPos.y = newRight.windowPos.y = nowY;
			ScanLine(newLeft, newRight);
			nowY--;
		}
	}
	void ScanLine(const V2F& left, const V2F& right) {

		int length = right.windowPos.x - left.windowPos.x;
		for (int i = 0; i < length; i++) {
			V2F v = V2F::lerp(left, right, (float)i / length);
			v.windowPos.x = left.windowPos.x + i;
			v.windowPos.y = left.windowPos.y;

			//深度测试
		/*	float depth = FrontBuffer->GetDepth(v.windowPos.x, v.windowPos.y);
			if (v.windowPos.z <= depth) {

				float z = v.Z;
				v.worldPos /= z;
				v.normal /= z;
				v.texcoord /= z;
				v.color /= z;

				FrontBuffer->WritePoint(v.windowPos.x, v.windowPos.y, currentMat->shader->FragmentShader(v));
				if (depthWrite)
					FrontBuffer->WriteDepth(v.windowPos.x, v.windowPos.y, v.windowPos.z);
			}*/
			FrontBuffer->WritePoint(v.windowPos.x, v.windowPos.y, shader->FragmentShader(v));
		}
	}
	//bresenhamLine 画线算法
	void DrawLine(const V2F& from, const V2F& to)
	{
		int dx = to.windowPos.x - from.windowPos.x;
		int dy = to.windowPos.y - from.windowPos.y;
		int Xstep = 1, Ystep = 1;
		if (dx < 0)
		{
			Xstep = -1;
			dx = -dx;
		}
		if (dy < 0)
		{
			Ystep = -1;
			dy = -dy;
		}
		int currentX = from.windowPos.x;
		int currentY = from.windowPos.y;
		V2F tmp;
		//斜率小于1
		if (dy <= dx)
		{
			int P = 2 * dy - dx;
			for (int i = 0; i <= dx; ++i)
			{
				tmp = V2F::lerp(from, to, ((float)(i) / dx));
				FrontBuffer->WritePoint(currentX, currentY, glm::vec4(255, 0, 0, 0));
				currentX += Xstep;
				if (P <= 0)
					P += 2 * dy;
				else
				{
					currentY += Ystep;
					P += 2 * (dy - dx);
				}
			}
		}
		//斜率大于1，利用对称性画
		else
		{
			int P = 2 * dx - dy;
			for (int i = 0; i <= dy; ++i)
			{
				tmp = V2F::lerp(from, to, ((float)(i) / dy));
				FrontBuffer->WritePoint(currentX, currentY, glm::vec4(255, 0, 0, 0));
				currentY += Ystep;
				if (P <= 0)
					P += 2 * dx;
				else
				{
					currentX += Xstep;
					P -= 2 * (dy - dx);
				}
			}
		}
	}
#pragma endregion Rasterization

};
#endif