//#include <limits>
//#include "model.h"
//#include "myGL.h"
//
//constexpr int width = 800; // output image size
//constexpr int height = 800;
//
//FrameBuffer* FrontBuffer;
//
//constexpr vec3 light_dir{ 1,1,1 }; // light source
//constexpr vec3       eye{ 1,1,3 }; // camera position
//constexpr vec3    center{ 0,0,0 }; // camera direction
//constexpr vec3        up{ 0,1,0 }; // camera up vector
//
//extern mat4 ModelView; // "OpenGL" state matrices
//extern mat4 Projection;
//
//struct Shader : IShader {
//	Model& model;
//	vec3 uniform_l;       // light direction in view coordinates
//	mat2x3 varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
//	mat3 varying_nrm; // normal per vertex to be interpolated by FS
//	mat3 view_tri;    // triangle in view coordinates
//
//	Shader(Model& m) : model(m) {
//		uniform_l = glm::normalize(vec3((ModelView * embed(light_dir, 0.)))); // transform the light vector to view coordinates
//	}
//
//	virtual void vertex(const int iface, const int nthvert, vec4& gl_Position) {
//		varying_uv[nthvert] = embed(model.uv(iface, nthvert));
//		varying_nrm[nthvert] = (nthvert, vec3(glm::inverse(ModelView) * embed(model.normal(iface, nthvert), 0.)));
//		gl_Position = ModelView * embed(model.vert(iface, nthvert));
//		view_tri[nthvert] = vec3(gl_Position);
//		gl_Position = Projection * gl_Position;
//	}
//
//	virtual bool fragment(const vec3 bar, vec4& gl_FragColor) {
//		vec3 bn = normalize(varying_nrm * bar); // per-vertex normal interpolation
//		vec2 uv = varying_uv * bar; // tex coord interpolation
//
//		// for the math refer to the tangent space normal mapping lecture
//		// https://github.com/ssloy/tinyrenderer/wiki/Lesson-6bis-tangent-space-normal-mapping
//		mat3 AI = inverse(mat3{ {view_tri[1] - view_tri[0], view_tri[2] - view_tri[0], bn} });
//		vec3 i = AI * vec3{ varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0 };
//		vec3 j = AI * vec3{ varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0 };
//		mat3 B = transpose(mat3{ {normalize(i), normalize(j), bn} });
//
//		vec3 n = normalize(B * model.normal(uv)); // transform the normal from the texture to the tangent space
//		double diff = std::max(float(0.), dot(n, uniform_l)); // diffuse light intensity
//		vec3 r = normalize(n * (n * uniform_l) * float(2.) - uniform_l); // reflected light direction, specular mapping is described here: https://github.com/ssloy/tinyrenderer/wiki/Lesson-6-Shaders-for-the-software-renderer
//		double spec = std::pow(std::max(-r.z, float(0.)), 5 + sample2D(model.specular(), uv)[0]); // specular intensity, note that the camera lies on the z-axis (in view), therefore simple -r.z
//
//		vec4 c = sample2D(model.diffuse(), uv);
//		for (int i : {0, 1, 2})
//			gl_FragColor[i] = std::min<int>(10 + c[i] * (diff + spec), 255); // (a bit of ambient light, diff + spec), clamp the result
//
//		return false; // the pixel is not discarded
//	}
//};
//
//
//void Init() {
//	if (FrontBuffer)
//		delete FrontBuffer;
//	FrontBuffer = new FrameBuffer(width, height);
//}
//
//Model* loadModel(const char* filename) {
//	return new Model(filename);
//}
//
//int drawModel(Model* model) {
//	lookat(eye, center, up);                            // build the ModelView matrix
//	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4); // build the Viewport matrix
//	projection(length(eye - center));                    // build the Projection matrix
//	std::vector<double> zbuffer(width * height, std::numeric_limits<double>::max());
//
//	//for (int m = 1; m < argc; m++) { // iterate through all input objects
//	//	Model model(argv[m]);
//	//	Shader shader(model);
//	//	for (int i = 0; i < model.nfaces(); i++) { // for every triangle
//	//		vec4 clip_vert[3]; // triangle coordinates (clip coordinates), written by VS, read by FS
//	//		for (int j : {0, 1, 2})
//	//			shader.vertex(i, j, clip_vert[j]); // call the vertex shader for each triangle vertex
//	//		triangle(clip_vert, shader, framebuffer, zbuffer); // actual rasterization routine call
//	//	}
//	//}
//
//	Shader shader(*model);
//	for (int i = 0; i < model->nfaces(); i++) { // for every triangle
//		vec4 clip_vert[3]; // triangle coordinates (clip coordinates), written by VS, read by FS
//		for (int j : {0, 1, 2})
//			shader.vertex(i, j, clip_vert[j]); // call the vertex shader for each triangle vertex
//		triangle(clip_vert, shader, *FrontBuffer, zbuffer); // actual rasterization routine call
//	}
//
//	return 0;
//}
