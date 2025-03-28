#pragma once
#include "Layer.h"
#include "MatchPoints.h"
#include "FileManager.h"
#include "Utils/Log.h"
#include <vector>
#include "Wall.h"


extern std::vector<Wall> pano_0_walls;
extern std::vector<Wall> pano_1_walls;
class ToolLayer : public Layer
{
public:
	ToolLayer();
	virtual void OnUIRender() override;
	static MatchPoints s_MatchPoints;
	static FileManager s_FileManager;
	static Log s_TextLog;

	//try all possible wall-wall matchings and report the best one (w.r.t. gt pose)
	//best_error: the best error (rotation angle error and translation angle error, in degree)
	bool TryAll(std::string& left_img, std::string& right_img, int method_type, glm::vec2 &best_error);
	bool eval();
	static void drawWalls(std::vector<Wall>& walls,ImDrawList* draw_list, ImVec2 viewport_bound, float m_ratio, bool isSecondPano)
	{
		for (auto& w : walls)
		{
			w.RenderWall(draw_list,viewport_bound,m_ratio,isSecondPano);
		}
	}
private:
	std::vector<glm::vec3> m_PanoPos_gt;
};

//populate a matchings by: 1) subdividing current matching in circular or not way
//2) generte more matchings on adjacent "ceiling" and "floor" planes
void PopulateMatching(MatchPoints& match_points, bool circular);

//add a line to g_texts_to_show
void AddTextToShow(const char* str);