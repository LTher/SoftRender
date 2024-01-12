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

	void drawModel(const char* filename) {
		//Model* model = new Model("obj/african_head.obj");
		Model* model = new Model("obj/diablo3_pose.obj");
		
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
			for (int k = 0; k < 3; k++) {
				uv[k] = model->uv(i, k);
			}
			triangle(model, pts, zbuffer, uv, glm::vec4(intensity, intensity, intensity, intensity));

			//randnom color
			//std::default_random_engine e;
			//std::uniform_int_distribution<int> u(0, 255); // 左闭右闭区间
			//e.seed(i);
			//triangle(pts, zbuffer, glm::vec4(u(e), u(e), u(e), u(e)));

		}

		delete model;
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

	void triangle(Model* model, glm::vec3* pts, float* zbuffer, glm::vec2* uv, glm::vec4 intensity) {
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
				glm::vec3 uv_screen = barycentric(glm::vec3(uv[0], 0), glm::vec3(uv[1], 0), glm::vec3(uv[2], 0), P);
				if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
				P.z = 0;
				glm::vec2 uvP(0, 0);
				for (int i = 0; i < 3; i++) {
					P.z += pts[i][2] * bc_screen[i];
					uvP.x += uv[i][0] * bc_screen[i];
					uvP.y += uv[i][1] * bc_screen[i];
				}
				//uvP = uv[0];
				if (zbuffer[int(P.x + P.y * Width)] < P.z) {
					zbuffer[int(P.x + P.y * Height)] = P.z;
					glm::vec4 color = model->diffuse(uvP);
					FrontBuffer->WritePoint(P.x, P.y, color* intensity);
				}
			}
		}
	}

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

#pragma WireframeRendering



#pragma endregion WireframeRendering

#endif