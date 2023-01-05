#pragma once

#include <opencv2\opencv.hpp>

#include "mesh.h"

mesh gen_mesh_circle(const float radius, const unsigned int num_segments);

mesh gen_mesh_floor(const cv::Mat& map, const float brick_size);
mesh gen_mesh_brick(const float brick_size);
mesh gen_mesh_gun(void);
void gen_mesh_magazines(std::vector<mesh> & magazines);