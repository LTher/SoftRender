#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <glm/glm.hpp>
#include<opencv2/opencv.hpp>

class Model {
private:
	std::vector<glm::vec3> verts_;
	std::vector<std::vector<glm::vec3> > faces_;
	std::vector<glm::vec3> norms_;
	std::vector<glm::vec2> uv_;
	cv::Mat diffusemap_;
	void load_texture(std::string filename, const char* suffix, cv::Mat& img);
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	glm::vec3 vert(int i);
	glm::vec3 vert(const int iface, const int nthvert) const;
	glm::vec3 normal(int i);
	glm::vec3 normal(const int iface, const int nthvert) const;
	glm::vec2 uv(int iface, int nvert);
	glm::vec4 diffuse(glm::vec2 uv);
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__