#include "../Public/Model.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "../Public/Tga.h"

Model::Model(const char* filename) : verts_(), faces_(), norms_(), uv_() {
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
			float temp[3];
			for (int i = 0; i < 3; i++) iss >> temp[i];
			glm::vec3 v(temp[0], temp[1], temp[2]);

			verts_.push_back(v);
		}
		else if (!line.compare(0, 3, "vn ")) {
			iss >> trash >> trash;
			glm::vec3 n;
			for (int i = 0; i < 3; i++) iss >> n[i];
			norms_.push_back(n);
		}
		else if (!line.compare(0, 3, "vt ")) {
			iss >> trash >> trash;
			glm::vec2 uv;
			for (int i = 0; i < 2; i++) iss >> uv[i];
			uv_.push_back(uv);
		}
		else if (!line.compare(0, 2, "f ")) {
			std::vector<glm::vec3> f;
			glm::vec3 tmp;
			iss >> trash;
			while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
				for (int i = 0; i < 3; i++) tmp[i]--; // in wavefront obj all indices start at 1, not zero
				f.push_back(tmp);
			}
			faces_.push_back(f);
		}
	}
	std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
	load_texture(filename, "_diffuse.png", diffusemap_);
}

Model::~Model() {
}

int Model::nverts() {
	return (int)verts_.size();
}

int Model::nfaces() {
	return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
	std::vector<int> face;
	for (int i = 0; i < (int)faces_[idx].size(); i++) face.push_back(faces_[idx][i][0]);
	return face;
}

glm::vec3 Model::vert(int i) {
	return verts_[i];
}

void Model::load_texture(std::string filename, const char* suffix, cv::Mat& img) {
	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	if (dot != std::string::npos) {
		texfile = texfile.substr(0, dot) + std::string(suffix);
		//std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
		//img.flip_vertically();

		/*Tga tgaImg = Tga(texfile.c_str());
		cv::Mat tempImg(tgaImg.GetHeight(), tgaImg.GetWidth(), CV_8UC3);*/
		//memcpy(tempImg.data, tgaImg.GetPixels().data(), tgaImg.GetHeight() * tgaImg.GetWidth() * 3);

		img = cv::imread(texfile);

		//flip(img, img, -1);
		/*cv::imshow("texture", img);
		cv::waitKey();*/
	}
}

glm::vec4 Model::diffuse(glm::vec2 uv) {
	//return diffusemap_.get(uv.x, uv.y);
	//cv::Vec4b cvColor = diffusemap_.at<cv::Vec4b>(int(uv.x), int(uv.y));
	// 
	// opencv 行列顺序不同需要改写
	cv::Vec3b cvColor = diffusemap_.at<cv::Vec3b>(int(diffusemap_.cols -uv.y), int(uv.x));

	return glm::vec4(cvColor[0], cvColor[1], cvColor[2], 1);
}

glm::vec2 Model::uv(int iface, int nvert) {
	int idx = faces_[iface][nvert][1];
	return glm::vec2(uv_[idx].x * diffusemap_.cols, uv_[idx].y * diffusemap_.rows);
}

glm::vec3 Model::normal(int iface, int nvert) {
	int idx = faces_[iface][nvert][2];
	return glm::normalize(norms_[idx]);
}