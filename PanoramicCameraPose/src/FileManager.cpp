#include "pch.h"
#include "FileManager.h"
#include "stb_image.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static void roll_image(unsigned char* data, int width, int height, int channels, int roll_amount, int axis) {
	if (axis == 1) {
		// Horizontal rolling (shift within rows)
		unsigned char* row_buffer = (unsigned char*)malloc(width * channels);

		for (int y = 0; y < height; y++) {
			unsigned char* row = data + y * width * channels;

			// Copy last 'roll_amount' pixels to the buffer
			memcpy(row_buffer, row + (width - roll_amount) * channels, roll_amount * channels);

			// Shift the remaining pixels to the right
			memmove(row + roll_amount * channels, row, (width - roll_amount) * channels);

			// Copy the buffer back to the start of the row
			memcpy(row, row_buffer, roll_amount * channels);
		}

		free(row_buffer);

	}
	else if (axis == 0) {
		// Vertical rolling (shift entire rows)
		unsigned char* col_buffer = (unsigned char*)malloc(width * channels * roll_amount);

		// Copy last 'roll_amount' rows to the buffer
		memcpy(col_buffer, data + (height - roll_amount) * width * channels, width * channels * roll_amount);

		// Shift the remaining rows down
		memmove(data + roll_amount * width * channels, data, (height - roll_amount) * width * channels);

		// Copy the buffer back to the top
		memcpy(data, col_buffer, width * channels * roll_amount);

		free(col_buffer);
	}
	else {
		printf("Invalid axis value! Use 0 for vertical or 1 for horizontal.\n");
	}
}

bool FileManager::LoadTextureFromFile(const std::filesystem::path& filepath, GLuint* out_texture, int* out_width, int* out_height,int offset)
{
	std::filesystem::path p = filepath;
	std::filesystem::path p_rolled = filepath;

	if (std::filesystem::is_directory(p)) // bug fixed! aim.png cannot rendered without this if condition!
	{
		p = p / "color.jpg";
		p_rolled = p_rolled / "rolled_color.jpg";
	}
	auto s = p.string();
	auto s1 = p_rolled.string();

	const char* filename = s.c_str();
	const char* p_rolled_c = s1.c_str();

	// Load from file
	int image_width = 0;
	int image_height = 0;
	int channels = 0;
	//stbi_set_flip_vertically_on_load(true);
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &channels, 0);

	//roll_image(image_data, image_width, image_height, channels, offset, 1);
	//int quality = 90;  // JPEG quality, between 1 (worst) and 100 (best)
	//if (stbi_write_jpg(p_rolled_c, image_width, image_height, channels, image_data, quality)) {
	//	printf("Image saved as: %s\n", p_rolled_c);
	//}
	//else {
	//	printf("Failed to save image as JPEG.\n");
	//}

	if (image_data == NULL)
		return false;
	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	if (channels == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

	stbi_image_free(image_data);

	*out_texture = image_texture;
	if (out_width != nullptr && out_height != nullptr)
	{
		*out_width = image_width;
		*out_height = image_height;
	}
	
	return true;
}

bool FileManager::LoadCornersFromFile(const std::filesystem::path& filepath, CornerType cornerType)
{
	
	
	std::ifstream file(filepath);
	std::string str;
	std::vector<glm::vec2> corner_pixels;
	std::vector<glm::vec3> positions;
	if (!file.is_open())
	{
		std::cout << filepath << "\nPredicted corner points from HorizonNet not found!\n";
		return false;
	}

	while (std::getline(file, str))
	{
		// read pano01 corner pixels
		std::istringstream iss(str);
		glm::vec2 corner_pixel;
		glm::vec3 pos;
		iss >> corner_pixel.x >> corner_pixel.y >> pos.x >> pos.y >> pos.z;

		corner_pixels.push_back(corner_pixel);
		positions.push_back(pos);
		
	}

	switch (cornerType)
	{
	case CornerType::Primary:
		m_pano01_corners.ClearPoints();
		m_pano01_corners.AddPoints(corner_pixels, positions);
		break;
	case CornerType::Secondary:
		m_pano02_corners.ClearPoints();
		m_pano02_corners.AddPoints(corner_pixels, positions);
		break;
	case CornerType::Transformed:
		m_trans_corners.ClearPoints();
		m_trans_corners.AddPoints(corner_pixels, positions);
		break;
	default:
		std::cout << "No match corner Type!" << std::endl;
		return false;
	}
	
	return true;
}
