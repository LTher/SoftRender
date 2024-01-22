#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <glm/glm.hpp>
#include<opencv2/opencv.hpp>

using namespace glm;
using namespace cv;

class Model {
private:
	std::vector<vec3> verts{};     // array of vertices
	std::vector<vec2> tex_coord{}; // per-vertex array of tex coords
	std::vector<vec3> norms{};     // per-vertex array of normal vectors
	std::vector<int> facet_vrt{};
	std::vector<int> facet_tex{};  // per-triangle indices in the above arrays
	std::vector<int> facet_nrm{};
	Mat diffusemap{};         // diffuse color texture
	Mat normalmap{};          // normal map texture
	Mat specularmap{};        // specular map texture
	void load_texture(std::string filename, const char* suffix, cv::Mat& img);
public:
	//Model(const char* filename);
	Model(const std::string filename);
	~Model();
	int nverts() const;
	int nfaces() const;
	vec3 normal(const int iface, const int nthvert) const; // per triangle corner normal vertex
	vec3 normal(const vec2& uv) const;                     // fetch the normal vector from the normal map texture
	vec3 vert(const int i) const;
	vec3 vert(const int iface, const int nthvert) const;
	vec2 uv(const int iface, const int nthvert) const;
	//vec3 diffuse(const vec2& uv) const;
	const Mat& diffuse()  const { return diffusemap; }
	const Mat& specular() const { return specularmap; }
	const Mat& normal() const { return normalmap; }
	/*int nverts();
	int nfaces();
	glm::vec3 vert(int i);
	glm::vec2 uv(int iface, int nvert);
	glm::vec3 normal(int iface, int nvert);
	glm::vec4 diffuse(glm::vec2 uv);
	std::vector<int> face(int idx);*/
};

#endif //__MODEL_H__