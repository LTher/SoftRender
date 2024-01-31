#include <glad/glad.h>
#include <GLFW/glfw3.h>
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
			color[i] = std::min<float>(Gray, 255);
			//color[i] = std::min<float>(std::max(spec, 0.0f)*255, 255);
			//color[i] = std::min<float>(5 + color[i] + spec * 255, 255); // (a bit of ambient light, diff + spec), clamp the result
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

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n\0";


int main() {

	/* dw = new Draw(SCR_WIDTH, SCR_HEIGHT);
	 dw->Init();
	 Shader shader;
	 FrameBuffer FrontBuffer(SCR_WIDTH, SCR_HEIGHT);

	 ViewPortMatrix = GetViewPortMatrix(0, 0, SCR_WIDTH, SCR_HEIGHT);
	 FrontBuffer.Resize(SCR_WIDTH, SCR_HEIGHT);


	 Vertex V1(glm::vec3(-0.5, -0.5, 0), glm::vec4(255, 0, 0, 0));
	 Vertex V2(glm::vec3(0.5, -0.5, 0), glm::vec4(0, 255, 0, 0));
	 Vertex V3(glm::vec3(0, 0.5, 0), glm::vec4(0, 0, 255, 0));

	 V2F o1 = shader.VertexShader(V1);
	 V2F o2 = shader.VertexShader(V2);
	 V2F o3 = shader.VertexShader(V3);

	 o1.windowPos = ViewPortMatrix * o1.windowPos;
	 o2.windowPos = ViewPortMatrix * o2.windowPos;
	 o3.windowPos = ViewPortMatrix * o3.windowPos;*/


	 //glfwInit();
	 //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	 //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	 //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	 //GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
	 //if (window == NULL) {
	 //    cout << "Failed to create GLFW window" << endl;
	 //    glfwTerminate();
	 //    return -1;
	 //}
	 //glfwMakeContextCurrent(window);

	 //if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
	 //    std::cout << "Failed to initialize GLAD" << std::endl;
	 //    return -1;
	 //}

	 //glViewport(0, 0, 800, 600);

	 //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	 //while (!glfwWindowShouldClose(window)) {

	 //    //FrontBuffer.ClearColorBuffer(glm::vec4(0, 0, 0, 0));
	 //    dw->ScanLineTriangle(o1, o2, o3);
	 //    glDrawPixels(SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, dw->FrontBuffer->colorBuffer.data());
	 //    //dw->Show();
	 //    glFlush();

	 //    glfwSwapBuffers(window);
	 //    glfwPollEvents();
	 //}

	 //glfwTerminate();
	 //return 0;

	 // glfw: initialize and configure
	 // ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}


	// build and compile our shader program
	// ------------------------------------
	// vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// link shaders
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		 0.5f,  0.5f, 0.0f,  // top right
		 0.5f, -0.5f, 0.0f,  // bottom right
		-0.5f, -0.5f, 0.0f,  // bottom left
		-0.5f,  0.5f, 0.0f   // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);


	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	// -----------
	Init();

	Mat mat(SCR_HEIGHT, SCR_WIDTH, CV_8UC4, FrontBuffer->colorBuffer.data());; //载入图像到test
	//Mat mat = imread("ss1.png");
	imshow("CV", mat);
	//waitKey(0);

	int picIndex = 0;
	//Model* model = loadModel("obj/african_head/african_head.obj");
	Model* model = loadModel("obj/diablo3_pose/diablo3_pose.obj");
	//Model* model = dw->loadModel("obj/diablo3_pose.obj");
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//// draw our first triangle
		//glUseProgram(shaderProgram);
		//glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		////glDrawArrays(GL_TRIANGLES, 0, 6);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// glBindVertexArray(0); // no need to unbind it every time

		//dw->ScanLineTriangle(o1, o2, o3);
		drawModel(model);
		mat = Mat(SCR_HEIGHT, SCR_WIDTH, CV_8UC4, FrontBuffer->colorBuffer.data()).clone();
		flip(mat, mat, 0);
		//mat = imread("ss"+ std::to_string(picIndex%4+1)+".png").clone();
		imshow("CV", mat);
		picIndex++;
		cout << picIndex << endl;
		//if (picIndex > 20) {
		//    break;
		//    //return 0;
		//}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();

	delete model;

	return 0;
}


//int main() {
//
//    dw = new Draw(SCR_WIDTH, SCR_HEIGHT);
//    dw->Init();
//    Shader shader;
//    FrameBuffer FrontBuffer(SCR_WIDTH, SCR_HEIGHT);
//
//    ViewPortMatrix = GetViewPortMatrix(0, 0, SCR_WIDTH, SCR_HEIGHT);
//    FrontBuffer.Resize(SCR_WIDTH, SCR_HEIGHT);
//
//
//    Vertex V1(glm::vec3(-0.5, -0.5, 0), glm::vec4(255, 0, 0, 0));
//    Vertex V2(glm::vec3(0.5, -0.5, 0), glm::vec4(0, 255, 0, 0));
//    Vertex V3(glm::vec3(0, 0.5, 0), glm::vec4(0, 0, 255, 0));
//
//    V2F o1 = shader.VertexShader(V1);
//    V2F o2 = shader.VertexShader(V2);
//    V2F o3 = shader.VertexShader(V3);
//
//    o1.windowPos = ViewPortMatrix * o1.windowPos;
//    o2.windowPos = ViewPortMatrix * o2.windowPos;
//    o3.windowPos = ViewPortMatrix * o3.windowPos;
//
//
//    
//    // render loop
//    // -----------
//    Mat mat(SCR_HEIGHT, SCR_WIDTH, CV_8UC4, FrontBuffer.colorBuffer.data());; //载入图像到test
//    //Mat mat = imread("ss1.png");
//    imshow("CV", mat);
//    //waitKey(0);
//
//    int picIndex = 0;
//    while (1)
//    {
//        dw->ScanLineTriangle(o1, o2, o3);
//        mat = Mat(SCR_HEIGHT, SCR_WIDTH, CV_8UC4, dw->FrontBuffer->colorBuffer.data()).clone();
//        //mat = imread("ss"+ std::to_string(picIndex%4+1)+".png").clone();
//        imshow("CV", mat);
//    }
//    return 0;
//}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}