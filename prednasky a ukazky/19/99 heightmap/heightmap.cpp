class mesh
{
public:
	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec4 > colors;
  GLenum primitive_type = GL_TRIANGLES;

  ...
  
}

------------------------------

mesh height_map;

void init_mesh(void)
{
	// height map
	{
		std::string hm_file("heights.png");
		cv::Mat hmap = cv::imread(hm_file, cv::IMREAD_GRAYSCALE );
		
		if (hmap.empty())
		{
			std::cerr << "ERR: Height map empty? File:" << hm_file << std::endl;
		}

		height_map = HeightMap(hmap, 10); //image, step size
		std::cout << "Note: height map vertices: " << height_map.vertices.size() << std::endl;
	}
}

//return bottom left ST of subtexture
glm::vec2 get_subtex_st(const int x, const int y)
{
	return glm::vec2(x * 1.0f / 16, y * 1.0f / 16);
}

glm::vec2 get_subtex_by_height(float height)
{
	if (height > 0.9)
		return get_subtex_st(2, 11); //snow
	else if (height > 0.8)
		return get_subtex_st(3, 11); //ice
	else if (height > 0.5)
		return get_subtex_st(0, 14); //rock
	else if (height > 0.3)
		return get_subtex_st(2, 15); //soil
	else 
		return get_subtex_st(0, 11); //grass
}

mesh2 HeightMap(const cv::Mat& hmap, const unsigned int mesh_step_size)
{
	mesh2 mesh;
	glm::vec3 v;
	glm::vec4 c;

	if (hmap.empty())
		return mesh;

	std::cout << "Note: heightmap size:" << hmap.size << ", channels: " << hmap.channels() << std::endl;

	if (hmap.channels() != 1)
	{
		std::cerr << "WARN: requested 1 channel, got: " << hmap.channels() << std::endl;
	}

	// Create heightmap mesh from TRIANGLES in XZ plane, Y is UP (right hand rule)
	//
	//   3-----2
	//   |    /|
	//   |  /  |
	//   |/    |
	//   0-----1
	//
	//   012,023
	//

	for (unsigned int x_coord = 0; x_coord < (hmap.cols - mesh_step_size); x_coord += mesh_step_size)
	{
		for (unsigned int z_coord = 0; z_coord < (hmap.rows - mesh_step_size); z_coord += mesh_step_size)
		{
			// Get The (X, Y, Z) Value For The Bottom Left Vertex = 0
			glm::vec3 p0(x_coord, hmap.at<uchar>(cv::Point(x_coord, z_coord)), z_coord);
			// Get The (X, Y, Z) Value For The Bottom Right Vertex = 1
			glm::vec3 p1(x_coord + mesh_step_size, hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord)), z_coord);
			// Get The (X, Y, Z) Value For The Top Right Vertex = 2
			glm::vec3 p2(x_coord + mesh_step_size, hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size)), z_coord + mesh_step_size);
			// Get The (X, Y, Z) Value For The Top Left Vertex = 3
			glm::vec3 p3(x_coord, hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size)), z_coord + mesh_step_size);

			// Get max normalized height for tile, set texture accordingly
			// Grayscale image returns 0..256, normalize to 0.0f..1.0f by dividing by 256
			float max_h = std::max(hmap.at<uchar>(cv::Point(x_coord, z_coord)) / 256.0f,
				          std::max(hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size)) / 256.0f,
				          std::max(hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size)) / 256.0f,
					               hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord)) / 256.0f
							)));

			// Get texture coords in vertices, bottom left of geometry == bottom left of texture
			glm::vec2 tc0 = get_subtex_by_height(max_h);
			glm::vec2 tc1 = tc0 + glm::vec2(1.0f / 16, 0.0f);			//add offset for bottom right corner
			glm::vec2 tc2 = tc0 + glm::vec2(1.0f / 16, 1.0f / 16);  //add offset for top right corner
			glm::vec2 tc3 = tc0 + glm::vec2(0.0f, 1.0f / 16);       //add offset for bottom leftcorner

			//place vertices and ST to mesh
			mesh.vertices.emplace_back(vertex(p0, tc0));
			mesh.vertices.emplace_back(vertex(p1, tc1));
			mesh.vertices.emplace_back(vertex(p2, tc2));
			mesh.vertices.emplace_back(vertex(p3, tc3));

			// place indices
			mesh.indices.emplace_back(0, 1, 2, 0, 2, 3);
		}
	}

	mesh.primitive_type = GL_TRIANGLES;

	return mesh;
}
