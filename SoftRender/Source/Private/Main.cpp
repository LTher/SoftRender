#include <iostream>
#include "../Public/Shader.h"
#include "../Public/myGL.h"
#include "../Public/Model.h"
#include "../Public/Draw.h"
#include "../Public/FrameBuffer.h"
#include<opencv2/opencv.hpp>
using namespace cv;
using namespace glm;
using namespace std;


constexpr int width = 800; // output image size
constexpr int height = 800;

FrameBuffer* FrontBuffer;
FrameBuffer* ShadowFrontBuffer = new FrameBuffer(width, height, 14);

vec3 light_dir{ 1,1,1 }; // light source
constexpr vec3       eye{ 0,0,3 }; // camera position
constexpr vec3    center{ 0,0,0 }; // camera direction
constexpr vec3        up{ 0,1,0 }; // camera up vector

//extern mat4 ModelView; // "OpenGL" state matrices
//extern mat4 Projection;

struct Shader : IShader {
	Model& model;
	vec3 uniform_l;       // light direction in view coordinates
	mat3x2 varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
	mat3 varying_nrm; // normal per vertex to be interpolated by FS
	mat3 view_tri;    // triangle in view coordinates

	Shader(Model& m) : model(m) {
		uniform_l = glm::normalize(vec3((ModelView * embed(light_dir, 0.)))); // transform the light vector to view coordinates
	}

	virtual void vertex(const int iface, const int nthvert, vec4& gl_Position) {
		varying_uv[nthvert] = embed(model.uv(iface, nthvert));
		varying_nrm[nthvert] = (nthvert, vec3(glm::inverse(ModelView) * embed(model.normal(iface, nthvert), 0.)));
		gl_Position = ModelView * embed(model.vert(iface, nthvert));
		view_tri[nthvert] = vec3(gl_Position);
		gl_Position = Projection * gl_Position;
	}

	virtual bool fragment(const vec3 bar, vec4& gl_FragColor) {
		vec3 bn = normalize(varying_nrm * bar); // per-vertex normal interpolation
		vec2 uv = varying_uv * bar; // tex coord interpolation

		// for the math refer to the tangent space normal mapping lecture
		// https://github.com/ssloy/tinyrenderer/wiki/Lesson-6bis-tangent-space-normal-mapping
		mat3 AI = inverse(mat3{ {view_tri[1] - view_tri[0], view_tri[2] - view_tri[0], bn} });
		vec3 i = AI * vec3{ varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0 };
		vec3 j = AI * vec3{ varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0 };
		mat3 B = transpose(mat3{ {normalize(i), normalize(j), bn} });

		vec3 n = normalize(B * model.normal(uv)); // transform the normal from the texture to the tangent space
		double diff = std::max(float(0.), dot(n, uniform_l)); // diffuse light intensity
		vec3 r = normalize(n * (n * uniform_l) * float(2.) - uniform_l); // reflected light direction, specular mapping is described here: https://github.com/ssloy/tinyrenderer/wiki/Lesson-6-Shaders-for-the-software-renderer
		double spec = std::pow(std::max(-r.z, float(0.)), 5 + sample2D(model.specular(), uv)[0]); // specular intensity, note that the camera lies on the z-axis (in view), therefore simple -r.z

		vec4 c = sample2D(model.diffuse(), uv);
		for (int i : {0, 1, 2})
			gl_FragColor[i] = std::min<int>(10 + c[i] * (diff + spec), 255); // (a bit of ambient light, diff + spec), clamp the result

		return false; // the pixel is not discarded
	}
};

struct DepthShader : public IShader {
	float depth = 255.f;
	Model& model;
	mat<3, 3, float> varying_tri;

	DepthShader(Model& m) :model(m) {}

	virtual vec4 vertex(int iface, int nthvert) {
		vec4 gl_Vertex = embed(model.vert(iface, nthvert)); // read the vertex from .obj file
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;          // transform it to screen coordinates
		//varying_tri.set_col(nthvert, proj(gl_Vertex / gl_Vertex[3]));
		varying_tri[nthvert] = proj(gl_Vertex / gl_Vertex[3]);
		return gl_Vertex;
	}

	virtual bool fragment(vec3 bar, vec4& color) {
		vec3 p = varying_tri * bar;
		color = vec4(255, 255, 255, 255) * (p.z / depth);
		return false;
	}
};

struct GouraudShader : public IShader {
	Model& model;
	vec3 uniform_l;       // light direction in view coordinates
	mat<4, 4, float> uniform_M;   //  Projection*ModelView
	mat<4, 4, float> uniform_MIT; // (Projection*ModelView).invert_transpose()
	vec3 varying_intensity; // written by vertex shader, read by fragment shader
	mat3x2 varying_uv;

	GouraudShader(Model& m) : model(m) {
		uniform_l = glm::normalize(vec3((ModelView * embed(light_dir, 0.)))); // transform the light vector to view coordinates
		uniform_M = Projection * ModelView;
		uniform_MIT = transpose(inverse(Projection * ModelView));
	}

	virtual vec4 vertex(int iface, int nthvert) {
		vec4 gl_Vertex = embed(model.vert(iface, nthvert)); // read the vertex from .obj file
		varying_uv[nthvert] = model.uv(iface, nthvert);

		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;     // transform it to screen coordinates
		float cur_intensity = dot(model.normal(iface, nthvert), light_dir);
		varying_intensity[nthvert] = std::max(0.f, cur_intensity); // get diffuse lighting intensity
		return gl_Vertex;
	}

	virtual bool fragment(vec3 bar, vec4& color) {
		float intensity = dot(varying_intensity, bar);   // interpolate intensity for the current pixel
		vec2 uv = varying_uv * bar;
		vec4 c = sample2D(model.diffuse(), uv);

		vec3 n = normalize(proj(uniform_MIT * embed(model.normal(uv))));
		//vec3 n = normalize(proj(uniform_MIT * sample2D(model.normal(), uv)));
		vec3 l = normalize(proj(uniform_M * embed(light_dir)));
		vec3 r = normalize(-(n * dot(n, l) * 2.f) + l);   // reflected light
		float diff = std::max(0.f, dot(n, l));
		vec3 specular = sample2D(model.specular(), uv);
		int Gray = int(specular.r * 77 + specular.g * 151 + specular.b * 28) >> 8;
		float spec = pow(std::max(intensity, 0.0f), 32);

		float cur_intensity = dot(n, l) * 255;
		//if (intensity <= 0.f) return true;
		color = c * intensity; // well duh

		for (int i = 0; i < 3; i++) {
			//color[i] = std::min<float>(5 + c[i] * (diff + .6 * spec), 255); // (a bit of ambient light, diff + spec), clamp the result
			//color[i] = std::min<float>(Gray, 255);
			//color[i] = std::min<float>(std::max(spec, 0.0f)*255, 255);
			color[i] = std::min<float>(5 + color[i] + 0.6 * spec * 255, 255); // (a bit of ambient light, diff + spec), clamp the result
		}
		//color[3] = 255;
		return false;                              // no, we do not discard this pixel
	}
};

void Init() {
	if (FrontBuffer)
		delete FrontBuffer;
	FrontBuffer = new FrameBuffer(width, height, 7);
}

Model* loadModel(const char* filename) {
	return new Model(filename);
}

int drawModel(Model* model) {

	// shadow buffer
	lookat(light_dir, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(0.);
	DepthShader depthshader(*model);
	std::vector<double> shadowbuffer(width * height, std::numeric_limits<double>::min());
	vec4 screen_coords[3];
	for (int i = 0; i < model->nfaces(); i++) {
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = depthshader.vertex(i, j);
		}
		triangle(screen_coords, depthshader, *ShadowFrontBuffer, shadowbuffer);
	}




	lookat(eye, center, up);                            // build the ModelView matrix
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4); // build the Viewport matrix
	projection(-1.f / length(eye - center));                    // build the Projection matrix
	light_dir = normalize(light_dir);
	std::vector<double> zbuffer(width * height, std::numeric_limits<double>::min());

	//for (int m = 1; m < argc; m++) { // iterate through all input objects
	//	Model model(argv[m]);
	//	Shader shader(model);
	//	for (int i = 0; i < model.nfaces(); i++) { // for every triangle
	//		vec4 clip_vert[3]; // triangle coordinates (clip coordinates), written by VS, read by FS
	//		for (int j : {0, 1, 2})
	//			shader.vertex(i, j, clip_vert[j]); // call the vertex shader for each triangle vertex
	//		triangle(clip_vert, shader, framebuffer, zbuffer); // actual rasterization routine call
	//	}
	//}

	//Shader shader(*model);
	GouraudShader shader(*model);
	for (int i = 0; i < model->nfaces(); i++) { // for every triangle
		vec4 clip_vert[3]; // triangle coordinates (clip coordinates), written by VS, read by FS
		for (int j : {0, 1, 2})
			clip_vert[j] = shader.vertex(i, j);
		//shader.vertex(i, j, clip_vert[j]); // call the vertex shader for each triangle vertex
		triangle(clip_vert, shader, *FrontBuffer, zbuffer); // actual rasterization routine call
	}

	return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

int main() {
	// render loop
	// -----------
	Init();
	Mat mat;

	int picIndex = 0;
	//Model* model = loadModel("obj/african_head/african_head.obj");
	Model* model = loadModel("obj/diablo3_pose/diablo3_pose.obj");
	//Model* model = dw->loadModel("obj/diablo3_pose.obj");
	while (1)
	{
		drawModel(model);
		mat = Mat(SCR_HEIGHT, SCR_WIDTH, CV_8UC4, FrontBuffer->colorBuffer.data()).clone();
		flip(mat, mat, 0);
		imshow("CV", mat);
		picIndex++;
		cout << picIndex << endl;

		// 等待按键，退出循环
		if (waitKey(1) >= 0)
			break;
	}
	delete model;
	return 0;
}