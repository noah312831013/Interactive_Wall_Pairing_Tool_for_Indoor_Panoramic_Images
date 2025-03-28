#pragma once
#include "glm/glm.hpp"
#include <string>
#include <vector>

namespace Utils
{
	std::tuple<double, double, double, double> ParseStrToRT(std::string& str,double corRot=0);
	std::vector<std::string> split(std::string s);
	openMVG::geometry::Pose3 ParseStrToPose(std::string& str,double corRot=0);
	openMVG::geometry::Pose3 LoadM3DPose();
	openMVG::geometry::Pose3 LoadZInDPose();
	//rotation_error and translation_error: angle errors in degrees
	void EvaluationMetrics(const openMVG::geometry::Pose3& pose_gt, 
		const openMVG::geometry::Pose3& pose_est, float *rotation_error = NULL, float *translation_error = NULL);
}