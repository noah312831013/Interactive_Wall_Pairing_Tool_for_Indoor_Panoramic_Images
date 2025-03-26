#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/normalize_dot.hpp>
#include <glm/gtc/constants.hpp> 
#include <vector>
#include <imgui/imgui.h>
std::vector<glm::vec2> PosToSphericalBatch(const std::vector<glm::vec3>& points, bool flip_x);
std::vector<glm::vec2> SphericalToXYBatch(const std::vector<glm::vec2>& spherical_coords, float width, float height);
std::vector<ImVec2> ConvertToImVec2(const std::vector<glm::vec2>& glmVecs);
std::vector<glm::vec3> InterpolateOnUnitSphere(const glm::vec3& start, const glm::vec3& end, int sampleCount);
std::vector<glm::vec2> pix2uv(const std::vector<glm::vec2>& xy, int width);
std::vector<glm::vec2> uv2spherical(const std::vector<glm::vec2>& uv);
std::vector<glm::vec3> spherical2xyz(const std::vector<glm::vec2>& spherical);
std::vector<glm::vec3> pix2xyz(const std::vector<glm::vec2>& xy, int width);
std::vector<glm::vec2> spherical2pix(const std::vector<glm::vec2>& spherical, int width);
std::vector<glm::vec2> xyz2spherical(const std::vector<glm::vec3>& xyz);
std::vector<glm::vec2> xyz2pix(const std::vector<glm::vec3>& xyz, int width);