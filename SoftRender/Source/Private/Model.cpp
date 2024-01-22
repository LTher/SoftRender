#include "../Public/Model.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "../Public/Tga.h"

Model::Model(const std::string filename) {
	std::ifstream in;
	in.open(filename, std::ifstream::in);
	if (in.fail()) return;
	std::string line;
	while (!in.eof()) {
		std::getline(in, line);
		std::istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) {
			iss >> trash;
			vec3 v;
			for (int i = 0; i < 3; i++) iss >> v[i];
			verts.push_back(v);
		}
		else if (!line.compare(0, 3, "vn ")) {
			iss >> trash >> trash;
			vec3 n;
			for (int i = 0; i < 3; i++) iss >> n[i];
			norms.push_back(normalize(n));
		}
		else if (!line.compare(0, 3, "vt ")) {
			iss >> trash >> trash;
			vec2 uv;
			for (int i = 0; i < 2; i++) iss >> uv[i];
			tex_coord.push_back({ uv.x, 1 - uv.y });
		}
		else if (!line.compare(0, 2, "f ")) {
			int f, t, n;
			iss >> trash;
			int cnt = 0;
			while (iss >> f >> trash >> t >> trash >> n) {
				facet_vrt.push_back(--f);
				facet_tex.push_back(--t);
				facet_nrm.push_back(--n);
				cnt++;
			}
			if (3 != cnt) {
				std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
				return;
			}
		}
	}
	std::cerr << "# v# " << nverts() << " f# " << nfaces() << " vt# " << tex_coord.size() << " vn# " << norms.size() << std::endl;
	load_texture(filename, "_diffuse.png", diffusemap);
	load_texture(filename, "_nm_tangent.png", normalmap);
	load_texture(filename, "_spec.png", specularmap);
}

Model::~Model() {
}

int Model::nverts() const {
	return verts.size();
}

int Model::nfaces() const {
	return facet_vrt.size() / 3;
}

vec3 Model::vert(const int i) const {
	return verts[i];
}

vec3 Model::vert(const int iface, const int nthvert) const {
	return verts[facet_vrt[iface * 3 + nthvert]];
}

void Model::load_texture(std::string filename, const char* suffix, cv::Mat& img) {
	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	if (dot != std::string::npos) {
		texfile = texfile.substr(0, dot) + std::string(suffix);
		img = cv::imread(texfile);
		//flip(img, img, -1);
		/*cv::imshow("texture", img);
		cv::waitKey();*/
	}
}

vec3 Model::normal(const vec2& uvf) const {
	//vec4 c = normalmap.get(uvf[0] * normalmap.width(), uvf[1] * normalmap.height());

	//Vec3b c = normalmap.at<cv::Vec3b>(int(uvf.y * normalmap.cols), int(uvf.x * normalmap.rows));
	//return vec3((double)c[2], (double)c[1], (double)c[0]) * vec3(2. / 255.) - vec3{ 1,1,1 };
	Vec3b c = normalmap.at<cv::Vec3b>(int(uvf.y * normalmap.cols), int(uvf.x * normalmap.rows));
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f;
	return res;
}

vec2 Model::uv(const int iface, const int nthvert) const {
	return tex_coord[facet_tex[iface * 3 + nthvert]];
}

vec3 Model::normal(const int iface, const int nthvert) const {
	return norms[facet_nrm[iface * 3 + nthvert]];
}

//vec3 Model::diffuse(const vec2& uvf) const {
//	//vec4 c = normalmap.get(uvf[0] * normalmap.width(), uvf[1] * normalmap.height());
//
//	Vec3b c = diffusemap.at<cv::Vec3b>(int(diffusemap.cols - uvf.y * diffusemap.cols), int(uvf.x * diffusemap.rows));
//	return vec3((double)c[2], (double)c[1], (double)c[0]) * vec3(2. / 255.) - vec3{ 1,1,1 };
//}

//glm::vec4 Model::diffuse(glm::vec2 uv) {
//	//return diffusemap_.get(uv.x, uv.y);
//	//cv::Vec4b cvColor = diffusemap_.at<cv::Vec4b>(int(uv.x), int(uv.y));
//	// 
//	// opencv 行列顺序不同需要改写
//	cv::Vec3b cvColor = diffusemap_.at<cv::Vec3b>(int(diffusemap_.cols -uv.y), int(uv.x));
//
//	return glm::vec4(cvColor[0], cvColor[1], cvColor[2], 1);
//}
//
//glm::vec2 Model::uv(int iface, int nvert) {
//	int idx = faces_[iface][nvert][1];
//	return glm::vec2(uv_[idx].x * diffusemap_.cols, uv_[idx].y * diffusemap_.rows);
//}
//
//glm::vec3 Model::normal(int iface, int nvert) {
//	int idx = faces_[iface][nvert][2];
//	return glm::normalize(norms_[idx]);
//}