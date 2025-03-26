#include "pch.h"
#include "ToolLayer.h"
#include "imgui/imgui.h"
#include "PanoLayer.h"
#include <ctime>
#include <cmath>
#include <random>
#include "Solvers/Solver.h"
#include "Utils/WindowsPlatformUtils.h"
#include <glm/gtx/string_cast.hpp>
#include "Wall.h"
#include <regex>

const unsigned int IMG_WIDTH = 1024;
const unsigned int IMG_HEIGHT = 512;
std::vector<Wall> pano_0_walls;
std::vector<Wall> pano_1_walls;
std::pair <int, float> po_0 = { 0,0 };
std::pair <int, float> po_1 = { 0,0 };
double corRot = 0;
extern float deltaTime;
FileDialogs pano1path;
namespace fs = std::filesystem;
static std::vector<char> user_name(128, '\0');  // 設定緩衝區大小（128 字元）
// Timer var
static std::clock_t start;
static double elapsed = 0;

static int wall_0 = -1, wall_1 = -1; // Hold user input values
static int prev_wall_0 = -1, prev_wall_1 = -1; // 用來記錄前一次的值
static bool inputs_received = false; // Track if inputs are ready

std::random_device rd;  // 硬體隨機數產生器
std::mt19937 gen(rd()); // 使用 Mersenne Twister 引擎
std::uniform_int_distribution<int> distrib(0, 1); // 產生 0 或 1

int test = distrib(gen);

static void populateOnWall( MatchPoints& s_MatchPoints, const int& pri_wall, const int& sec_wall)
{
	s_MatchPoints.ClearPixel();
	const int order[4] = { 0, 1, 3, 2 };

	for (auto& i : order)
	{
		const ImU32 col = ImColor(ImVec4((rand() % 256) / 255.0f,
			(rand() % 256) / 255.0f,
			(rand() % 256) / 255.0f, 1.0f));
		s_MatchPoints.AddPoint(
			pano_0_walls[pri_wall].pixelCorners[i],
			pano_1_walls[sec_wall].pixelCorners[i],
			col, 10,
			pano_0_walls[pri_wall].xyzCorners[i],
			pano_1_walls[sec_wall].xyzCorners[i]
		);
	}
	PopulateMatching(s_MatchPoints, false);
}



// Function to list all folders under a given path
static std::vector<std::wstring> listFolders(const std::filesystem::path& rootPath) {
	std::vector<std::wstring> folderList;
	for (const auto& entry : std::filesystem::directory_iterator(rootPath)) {
		if (entry.is_directory()) {
			folderList.push_back(entry.path().filename().wstring());
		}
	}
	// Sort the folder list numerically
	std::sort(folderList.begin(), folderList.end(), [](const std::wstring& a, const std::wstring& b) {
		return std::stoi(a) < std::stoi(b);
		});
	return folderList;
}

// Main function to perform the required operations
static std::wstring processFilePath(const std::wstring& filepath) {
	try {
		// Convert the file path to a filesystem object
		std::filesystem::path filePathObj(filepath);

		// Step 1: Get the name of the parent folder (current_index)
		std::wstring current_index = filePathObj.parent_path().filename().wstring();
		std::wcout << L"Current Index (Parent Folder): " << current_index << std::endl;

		// Step 2: Get the ../../ path (root_path)
		std::filesystem::path root_path = filePathObj.parent_path().parent_path();
		//std::wcout << L"Root Path (../../): " << root_path.wstring() << std::endl;

		// Step 3: List all folders under root_path
		std::vector<std::wstring> folderList = listFolders(root_path);
		//std::wcout << L"Folders under Root Path:" << std::endl;
		//for (const auto& folder : folderList) {
		//	std::wcout << L" - " << folder << std::endl;
		//}

		// Step 4: Search for current_index and find the next index
		auto it = std::find(folderList.begin(), folderList.end(), current_index);
		if (it != folderList.end() && (it + 1) != folderList.end()) {
			std::wstring next_index = *(it + 1);
			std::wcout << L"Next Index: " << next_index << std::endl;

			// Step 5: Combine root_path and next_index to get next_path
			std::filesystem::path next_path = root_path / next_index;
			std::wcout << L"Next Path: " << next_path.wstring() << std::endl;

			return next_path.wstring();
		}
		else {
			std::wcout << L"No next folder found after " << current_index << std::endl;
			return L"";
		}
	}
	catch (const std::exception& e) {
		std::wcerr << L"Error: " << e.what() << std::endl;
		return L"";
	}
}
// Make the UI compact because there are so many fields
static void PushStyleCompact()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, (float)(int)(style.FramePadding.y * 0.60f)));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, (float)(int)(style.ItemSpacing.y * 0.60f)));
}

static void PopStyleCompact()
{
	ImGui::PopStyleVar(2);
}


static void get_paired_pano_path(const std::wstring& folder_path, std::wstring& pano1_path, std::wstring& pano2_path)
{
	// Define the pattern for "pano_RXXXXX" (starts with "pano_R" followed by digits)
	std::wregex pano_R_pattern(L"^pano_R.*");

	try {
		for (const auto& entry : fs::directory_iterator(folder_path)) {
			if (entry.is_directory()) {
				std::wstring folder_name = entry.path().filename().wstring();

				if (folder_name == L"pano_orig") {
					pano1_path = entry.path().wstring();
				}
				else if (std::regex_match(folder_name, pano_R_pattern)) {
					pano2_path = entry.path().wstring();
				}
			}
		}

		if (!pano1_path.empty()) {
			std::wcout << L"pano1_path: " << pano1_path << std::endl;
		}
		else {
			std::wcerr << L"Error: 'pano_orig' folder not found." << std::endl;
		}

		if (!pano2_path.empty()) {
			std::wcout << L"pano2_path: " << pano2_path << std::endl;
		}
		else {
			std::wcerr << L"Error: 'pano_RXXXXX' folder not found." << std::endl;
		}

	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "General error: " << e.what() << std::endl;
	}
}

//convert spherical coordinate (azimuth,zenith) and 3D position by the standard spherical coordinate formulas
//flip_x: flip x's sign for LED2-Net's coordinate space
static glm::vec2 PosToSpherical(glm::vec3 p, bool flip_x = true)
{
	p = glm::normalize(p);

	//flip x?
	if (flip_x)
	{
		p.x = -p.x;
		
	}
	else
	{
		// HorizonNet's coordinate space
		std::swap(p.x, p.y);
		p.y = -p.y;
	}

	glm::vec2 c;
	c.y = acos(p.z);  //zenith

	//azimuth:
	if (p.x > 0)
	{
		c.x = atan(p.y / p.x);
	}
	else if (p.x < 0 && p.y >= 0)
	{
		c.x = atan(p.y / p.x) + glm::pi<float>();
	}
	else if (p.x < 0 && p.y < 0)
	{
		c.x = atan(p.y / p.x) - glm::pi<float>();
	}
	else if (p.x == 0 && p.y > 0) 
	{
		c.x = glm::pi<float>() / 2;
	}
	else if (p.x == 0 && p.y < 0)
	{
		c.x = -glm::pi<float>() / 2;
	}
	else  //undefined
	{
		std::cout << "[PosToSpherical] azimuth undefined!" << std::endl;
		c.x = 0;
	}

	//azimuth don't be negative
	if (c.x < 0)
		c.x = 2 * glm::pi<float>() + c.x;

	return c;
}

//convert a standard spherical coord to LED2-net's spherical coord
static glm::vec2 SphericalToXY(glm::vec2 s, float width, float height)
{
	glm::vec2 xy;
	xy.x = s.x / (2 * glm::pi<float>()) * width;
	xy.y = s.y / (glm::pi<float>()) * height;

	return xy;
}

//texts to show
std::vector<std::string> g_texts_to_show;
void AddTextToShow(const char* str)
{
	g_texts_to_show.push_back(std::string(str));
}

ToolLayer::ToolLayer() : m_PanoPos_gt(IMG_WIDTH * IMG_HEIGHT)
{
	srand(time(0));

	// read pos txt
	std::ifstream file("assets/ground_truth/pano_ori_pos_gt.txt");
	std::string str;
	unsigned int ind = 0;
	while (std::getline(file, str))
	{
		std::istringstream iss(str);
		glm::vec3 pos;
		iss >> pos.x >> pos.y >> pos.z;
		m_PanoPos_gt[ind] = pos;
		ind++;
	}

	// set default filepath for test quickly
	s_FileManager.SetPano01Info("assets/test_data/ZInD/02/pano_orig");
	s_FileManager.SetPano02Info("assets/test_data/ZInD/02/pano_R38_506_T(0_672,-1_721,0_0)");
	
	//s_FileManager.SetPano01Info("assets/test_data/ZInD/75-100/019/pano_orig");
	//s_FileManager.SetPano02Info("assets/test_data/ZInD/75-100/019/pano_R1_049_T(0_467,0_052,-0_003)");

	//s_FileManager.SetPano01Info("assets/test_data/ZInD/50-75/021/pano_orig");
	//s_FileManager.SetPano02Info("assets/test_data/ZInD/50-75/021/pano_R89_981_T(0_582,0_351,0_003)");

	//s_FileManager.SetPano01Info("assets/test_data/ZInD/50-75/032/pano_orig");
	//s_FileManager.SetPano02Info("assets/test_data/ZInD/50-75/032/pano_R-148_452_T(-0_785,0_42,0_002)");

	//s_FileManager.SetPano01Info("assets/test_data/ZInD/50-75/054/pano_orig");
	//s_FileManager.SetPano02Info("assets/test_data/ZInD/50-75/054/pano_R89_427_T(-0_38,0_596,0_003)");

	//s_FileManager.SetPano01Info("assets/test_data/ZInD/50-75/123/pano_orig");
	//s_FileManager.SetPano02Info("assets/test_data/ZInD/50-75/123/pano_R0_364_T(0_318,-0_701,-0_001)");
}

void PopulateMatching(MatchPoints &match_points, bool circular)
{
	//add intermediate points on edges of the layout?
	//assume the original points are upper ones and then lower ones

	//new set of MatchPoints (to replace old one later)
	MatchPoints new_points;

	//test: "extrapolate" more points
	//assume there are 4 points in CCW order toward the viewer
	if (true && match_points.left_positions.size() == 4 && match_points.right_positions.size() == 4)
	{
		std::cout << "===========Enter============\n";
		
		std::vector<glm::vec3> new_left_points;
		std::vector<glm::vec2> new_left_pixels;
		std::vector<glm::vec3> new_right_points;
		std::vector<glm::vec2> new_right_pixels;

		for (int Case = 0; Case < 2; Case++)  //left and right panorama:
		{
			glm::vec3 p0, p1, p2, p3;  //the 4 corners of the plane in CCW order

			if (Case == 0)
			{
				p0 = match_points.left_positions[0];
				p1 = match_points.left_positions[1];
				p2 = match_points.left_positions[2];
				p3 = match_points.left_positions[3];
			}
			else
			{
				p0 = match_points.right_positions[0];
				p1 = match_points.right_positions[1];
				p2 = match_points.right_positions[2];
				p3 = match_points.right_positions[3];
			}

			//plane normal:
			glm::vec3 n = glm::cross(glm::normalize((p2 - p1)), glm::normalize((p1 - p0)));
			n = glm::normalize(n);
			
			//populate more points on the adjacent ceiling and floor planes:
			const int subd = 6;  //inclusive of two ends
			const float extrude_len = glm::length(p1 - p0) / 5;
			for (int s = 0; s < subd; s++)
			{
				//ceiling: from edge p0->p1, extrude along normal
				{
					glm::vec3 pos = (p0 + (p1 - p0) * (float)s / (float)(subd - 1)) + n * extrude_len;
					glm::vec2 pixel = SphericalToXY(PosToSpherical(pos, false), 1024, 512);
					if (Case == 0)
					{
						new_left_points.push_back(pos);
						new_left_pixels.push_back(pixel);
					}
					else
					{
						new_right_points.push_back(pos);
						new_right_pixels.push_back(pixel);
					}
				}

				//floor: from edge p2->p3, extrude along normal
				{
					glm::vec3 pos = (p2 + (p3 - p2) * (float)s / (float)(subd - 1)) + n * extrude_len;
					glm::vec2 pixel = SphericalToXY(PosToSpherical(pos, false), 1024, 512);
					if (Case == 0)
					{
						new_left_points.push_back(pos);
						new_left_pixels.push_back(pixel);
					}
					else
					{
						new_right_points.push_back(pos);
						new_right_pixels.push_back(pixel);
					}
				}
			}
		}

		for (int ii = 0; ii < new_left_points.size(); ii++)
		{
			const ImU32 col = ImColor(ImVec4(0, 1, 1, 1.0f));

			new_points.AddPoint(new_left_pixels[ii], new_right_pixels[ii], col, 10,
				new_left_points[ii], new_right_points[ii]);
		}
	}

	const int subd = 4;  //how many subdivided vertices to add alone an edge?
	int num_corners = match_points.left_positions.size() / 2;

	if (!circular)
	{
		//ceiling then floor:
		for (int Case = 0; Case < 2; Case++)
		{
			for (int i = 0; i < num_corners - 1; i++)
			{
				int offset = 0;
				if (Case == 1)
					offset += num_corners;  //the latter half

				//add the original point (on the start):
				new_points.AddPoint(match_points.left_pixels[offset + i],
					match_points.right_pixels[offset + i],
					match_points.v_color[offset + i],
					10,
					match_points.left_positions[offset + i],
					match_points.right_positions[offset + i]);

				//create and add the new intermediate points:
				glm::vec3 p0 = match_points.left_positions[offset + i];
				glm::vec3 p1 = match_points.left_positions[offset + i + 1];
				glm::vec3 P0 = match_points.right_positions[offset + i];
				glm::vec3 P1 = match_points.right_positions[offset + i + 1];
				for (int j = 0; j < subd; j++)
				{
					//left:
					glm::vec3 p = p0 + (p1 - p0) * (float)(j + 1) / (float)(subd + 1);
					//the point's 2d pixel pos?
					glm::vec2 xy = SphericalToXY(PosToSpherical(p, false), 1024, 512); // PosToSpherical sets false for HorizonNet's coordinate system

					//right:
					glm::vec3 P = P0 + (P1 - P0) * (float)(j + 1) / (float)(subd + 1);
					//the point's 2d pixel pos?
					glm::vec2 XY = SphericalToXY(PosToSpherical(P, false), 1024, 512); // PosToSpherical sets false for HorizonNet's coordinate system

					const ImU32 col = ImColor(ImVec4(1, 1, 1, 1.0f));

					//index of this point pair:
					std::pair index(offset + i, j + 1);

					new_points.AddPoint(xy, XY, col, 10, p, P);
				}

				//add the original point (on the end):
				new_points.AddPoint(match_points.left_pixels[offset + i + 1],
					match_points.right_pixels[offset + i + 1],
					match_points.v_color[offset + i + 1],
					10,
					match_points.left_positions[offset + i + 1],
					match_points.right_positions[offset + i + 1]);


			}
		}
	}
	else  //circular add
	{
		//ceiling then floor:
		for (int Case = 0; Case < 2; Case++)
		{
			for (int i = 0; i < num_corners; i++)
			{
				int offset = 0;
				if (Case == 1)
					offset += num_corners;  //the latter half

				//add the original point (on the begin):
				new_points.AddPoint(match_points.left_pixels[offset + i],
					match_points.right_pixels[offset + i],
					match_points.v_color[offset + i],
					10,
					match_points.left_positions[offset + i],
					match_points.right_positions[offset + i]);

				//create and add the new intermediate points:
				glm::vec3 p0 = match_points.left_positions[offset + i];
				glm::vec3 p1 = match_points.left_positions[offset + (i + 1) % num_corners];
				glm::vec3 P0 = match_points.right_positions[offset + i];
				glm::vec3 P1 = match_points.right_positions[offset + (i + 1) % num_corners];
				for (int j = 0; j < subd; j++)
				{
					//left:
					glm::vec3 p = p0 + (p1 - p0) * (float)(j + 1) / (float)(subd + 1);
					//the point's 2d pixel pos?
					glm::vec2 xy = SphericalToXY(PosToSpherical(p, false), 1024, 512); // PosToSpherical sets false for HorizonNet's coordinate system

					//right:
					glm::vec3 P = P0 + (P1 - P0) * (float)(j + 1) / (float)(subd + 1);
					//the point's 2d pixel pos?
					glm::vec2 XY = SphericalToXY(PosToSpherical(P, false), 1024, 512); // PosToSpherical sets false for HorizonNet's coordinate system

					const ImU32 col = ImColor(ImVec4(1, 1, 1, 1.0f));

					//index of this point pair:
					std::pair index(offset + i, j + 1);

					new_points.AddPoint(xy, XY, col, 10, p, P);
				}
			}
		}
	}

	match_points = new_points;
}

void ToolLayer::OnUIRender()
{
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
	ImGui::Begin("Tool");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	int ind = PanoLayer::s_left_pixel.y * IMG_WIDTH + PanoLayer::s_left_pixel.x;
	if (ind >= 0)
	{
		ImGui::Text("Position : %f %f %f", m_PanoPos_gt[ind].x, m_PanoPos_gt[ind].y, m_PanoPos_gt[ind].z);
		ImGui::Text("Left Pixel X: %f  Y: %f", PanoLayer::s_left_pixel.x, PanoLayer::s_left_pixel.y);
		ImGui::Text("Right Pixel X: %f  Y: %f", PanoLayer::s_right_pixel.x, PanoLayer::s_right_pixel.y);
	}

	// File Open
	{
		char buffer[2][256];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer[0], s_FileManager.GetPano01Filepath().filename().string().c_str(), sizeof(buffer[0]));
		std::strncpy(buffer[1], s_FileManager.GetPano02Filepath().filename().string().c_str(), sizeof(buffer[1]));
		
		ImGui::InputTextWithHint("UserName", "enter your name", user_name.data(), user_name.size());
		ImGui::SameLine();
		if (ImGui::Button("SwitchCase")) {
			test++;
			test %= 2;
			pano_0_walls.clear();
			pano_1_walls.clear();
		}
		// Pano 01
		ImGui::InputTextWithHint("Pano01", "open / drag and drop folder here...", buffer[0], sizeof(buffer[0]), ImGuiInputTextFlags_ReadOnly);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILES"))
			{
				const wchar_t* filepath = (const wchar_t*)payload->Data;
				if (fs::is_directory(filepath)) // only accept folder
				{
					s_FileManager.SetPano01Info(filepath);
					s_MatchPoints.ClearPixel();
				}
			}
			ImGui::EndDragDropTarget();
		}


		ImGui::SameLine();
		if (ImGui::Button("Open..."))
		{
			std::wstring filepath = pano1path.OpenFolder(pano1path);
			pano1path.previousPath = filepath;
			std::wstring p1, p2;
			get_paired_pano_path(filepath,p1,p2);
			if (!filepath.empty())
			{
				s_FileManager.SetPano01Info(p1); // set and load texture
				s_FileManager.SetPano02Info(p2);
				s_MatchPoints.ClearPixel();
				pano_0_walls.clear();
				pano_1_walls.clear();

			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Next"))
		{
			wall_0 = -1, wall_1 = -1; // Hold user input values
			prev_wall_0 = -1, prev_wall_1 = -1; // 用來記錄前一次的值
			std::wstring filepath = processFilePath(s_FileManager.GetPano01Filepath());
			pano1path.previousPath = filepath;
			std::wstring p1, p2;
			get_paired_pano_path(filepath, p1, p2);
			if (!filepath.empty())
			{
				s_FileManager.SetPano01Info(p1); // set and load texture
				s_FileManager.SetPano02Info(p2);
				s_MatchPoints.ClearPixel();
				pano_0_walls.clear();
				pano_1_walls.clear();

			}
		}
		// Pano 02
		ImGui::InputTextWithHint("Pano02", "open / drag and drop folder here...", buffer[1], sizeof(buffer[1]), ImGuiInputTextFlags_ReadOnly);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILES"))
			{
				const wchar_t* filepath = (const wchar_t*)payload->Data;
				if (fs::is_directory(filepath))
				{
					s_FileManager.SetPano02Info(filepath);
					s_MatchPoints.ClearPixel();
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();
		ImGui::PushID(1); // avoid imgui same name issue
		if (ImGui::Button("Open..."))
		{
			//std::string filepath = FileDialogs::OpenFile("JPG (*.jpg;*.jpeg;*.jpe;*.jfif)\0*.jpg;*.jpeg;*.jpe;*.jfif\0");
			std::wstring filepath = pano1path.OpenFolder(pano1path);
			if (!filepath.empty())
			{
				s_FileManager.SetPano02Info(filepath);
				s_MatchPoints.ClearPixel();
				pano_1_walls.clear();
			}
		}
		ImGui::PopID();
	}
	ImGui::Separator();
	if (ImGui::Button("Match"))  //add a user-specified matching
	{
		unsigned int ind = PanoLayer::s_left_pixel.y * IMG_WIDTH + PanoLayer::s_left_pixel.x;
		const ImU32 col = ImColor(ImVec4((rand() % 256) / 255.0f, (rand() % 256) / 255.0f, (rand() % 256) / 255.0f, 1.0f));
		/* Now Matching point by human doesn't have position information */

		s_MatchPoints.AddPoint(PanoLayer::s_left_pixel, PanoLayer::s_right_pixel, col, 100, PanoLayer::s_left_pos, PanoLayer::s_right_pos);
		s_MatchPoints.show();
	}

	if (ImGui::Button("Corner->Match"))
	{
		s_MatchPoints.CovertCornerToMatch(s_FileManager.GetPano01Corners(), s_FileManager.GetPano02Corners());
		s_MatchPoints.show();
	}

	static int selectIndex = -1;

	ImGui::SameLine();
	if (ImGui::Button("Delete Match"))
	{
		if (selectIndex >= 0)
		{
			//delete #selectIndex s_MatchPoints record
			s_MatchPoints.DeletePoint(selectIndex);
			s_MatchPoints.show();
			selectIndex = -1;
			PanoLayer::s_left_pixel = glm::vec2(-1, -1);
			PanoLayer::s_right_pixel = glm::vec2(-1, -1);
		}
	}

	if (ImGui::Button("Show Transformed corners"))
	{
		s_FileManager.GetTransformCorners().isLoad = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		s_MatchPoints.ClearPixel();
		s_FileManager.DisableShowCorners();
		pano_0_walls.clear();
		pano_1_walls.clear();
	}

	// match table
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

		PushStyleCompact();
		ImGui::CheckboxFlags("ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
		PopStyleCompact();

		// When using ScrollX or ScrollY we need to specify a size for our table container!
		// Otherwise by default the table will fit all available space, like a BeginChild() call.
		ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 12);
		if (ImGui::BeginTable("Matching points", 4, flags, outer_size))
		{
			ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
			ImGui::TableSetupColumn("Match #", ImGuiTableColumnFlags_None);
			ImGui::TableSetupColumn("Pano_01", ImGuiTableColumnFlags_None);
			ImGui::TableSetupColumn("Pano_02", ImGuiTableColumnFlags_None);
			//ImGui::TableSetupColumn("Position_groundtruth", ImGuiTableColumnFlags_None);
			ImGui::TableHeadersRow();

			for (int row = 0; row < s_MatchPoints.size(); ++row)
			{
				ImGui::TableNextRow();
				auto& L = s_MatchPoints.left_pixels[row];
				auto& R = s_MatchPoints.right_pixels[row];
				auto& p = s_MatchPoints.left_positions[row];
				const bool matchIsSelectd = selectIndex == row;
				for (int column = 0; column < 4; column++)
				{
					ImGui::TableSetColumnIndex(column);
					switch (column)
					{
					case 0:
					{
						// Selectable for inspecting matching point
						char label[32];
						sprintf(label, "%d", row);
						ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
						if (ImGui::Selectable(label, matchIsSelectd, selectable_flags, ImVec2(0, TEXT_BASE_HEIGHT)))
						{
							PanoLayer::s_left_pixel = L;
							PanoLayer::s_right_pixel = R;
							selectIndex = (selectIndex == row ? -1 : row);
						}
						//ImGui::Text("Match %d", row);
						break;
					}
					case 1:
						ImGui::Text("(%.2f, %.2f)", L.x, L.y);
						break;
					case 2:
						ImGui::Text("(%.2f, %.2f)", R.x, R.y);
						break;
					case 3:
						ImGui::Text("(%.4f, %.4f, %.4f)", p.x, p.y, p.z);
						break;
					default:
						ImGui::Text("Hello %d,%d", column, row);
						break;
					}
				}
			}

			ImGui::EndTable();
		}
	}

	static int current_corner_type = 1;
	// Load corners
	if (ImGui::Button("Load Corners"))
	{
		const char* corner_filename[] = { "pred_corner_raw.txt", "pred_corner.txt", "pred_corner_peak.txt", "pred_corner_LED2Net.txt", "pred_corner_LGT.txt"};
		const char* corner_same_filename[] = { "pred_corner_raw.txt", "pred_corner_same.txt", "pred_corner_peak.txt", "pred_corner_LED2Net.txt", "pred_corner_LGT.txt" };
		std::filesystem::path corner_path01 = s_FileManager.GetPano01Filepath() / corner_filename[current_corner_type];
		std::filesystem::path corner_path02 = s_FileManager.GetPano02Filepath() / corner_filename[current_corner_type];
		std::filesystem::path transCornersPath = s_FileManager.GetPano02Filepath().parent_path() / "trans_corner.txt";
		
		if (corner_path01.empty() || corner_path02.empty())
		{
			std::cout << "Please Upload " << corner_filename[current_corner_type] << std::endl;
		}
		else if (transCornersPath.empty())
		{
			std::cout << "Please Upload trans_corner.txt" << std::endl;
		}
		else
		{
			s_MatchPoints.ClearPixel();
			s_FileManager.SetPano01Corners(corner_path01);
			s_FileManager.SetPano02Corners(corner_path02);
			s_FileManager.SetTransformCorners(transCornersPath);
			s_FileManager.GetPano01Corners().isLoad = true;
			s_FileManager.GetPano02Corners().isLoad = true;
			
			//====== Following code segments for loading corners as matching points======
			
			std::ifstream file(corner_path01);
			std::ifstream file2(corner_path02);
			std::string str, str2;
			while (std::getline(file, str) && std::getline(file2, str2))
			{
				// read pano01 corner pixels
				std::istringstream iss(str);
				glm::vec2 corner_pixel;
				glm::vec3 pos;
				iss >> corner_pixel.x >> corner_pixel.y >> pos.x >> pos.y >> pos.z;
				std::cout << corner_pixel.x << std::endl;
				// read pano02 corner pixels
				std::istringstream iss2(str2);
				glm::vec2 corner_pixel2;
				glm::vec3 pos2;
				iss2 >> corner_pixel2.x >> corner_pixel2.y >> pos2.x >> pos2.y >> pos2.z;

				const ImU32 col = ImColor(ImVec4((rand() % 256) / 255.0f, (rand() % 256) / 255.0f, (rand() % 256) / 255.0f, 1.0f));
				
				//which layout side, 0-th point on the side  
				std::pair<int, int> index = std::make_pair(s_MatchPoints.size(), 0);

				s_MatchPoints.AddPoint(corner_pixel, corner_pixel2, col, 10, pos, pos2);
			}
			
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("load Walls")) 
	{
		const char* corner_filename[] = { "pred_corner_raw.txt", "pred_corner.txt", "pred_corner_peak.txt", "pred_corner_LED2Net.txt", "pred_corner_LGT.txt" };
		const char* corner_same_filename[] = { "pred_corner_raw.txt", "pred_corner_same.txt", "pred_corner_peak.txt", "pred_corner_LED2Net.txt", "pred_corner_LGT.txt" };
		std::filesystem::path corner_path01 = s_FileManager.GetPano01Filepath() / corner_filename[current_corner_type];
		std::filesystem::path corner_path02 = s_FileManager.GetPano02Filepath() / corner_filename[current_corner_type];
		std::filesystem::path TW_path01 = s_FileManager.GetPano01Filepath() / "TW_.txt";
		std::filesystem::path TW_path02 = s_FileManager.GetPano02Filepath() / "TW_.txt";
		std::filesystem::path transCornersPath = s_FileManager.GetPano02Filepath().parent_path() / "trans_corner.txt";

		if (corner_path01.empty() || corner_path02.empty() || TW_path01.empty() || TW_path02.empty())
		{
			std::cout << "Please Upload " << corner_filename[current_corner_type] << std::endl;
		}
		else if (transCornersPath.empty())
		{
			std::cout << "Please Upload trans_corner.txt" << std::endl;
		}
		else
		{
			std::vector <float> TW_0;
			std::vector <float> TW_1;
			// hide non-trivial indicator
			float threshold = 0.7;
			s_MatchPoints.ClearPixel();
			pano_0_walls.clear();
			pano_1_walls.clear();
			s_FileManager.SetPano01Corners(corner_path01);
			s_FileManager.SetPano02Corners(corner_path02);
			s_FileManager.SetTransformCorners(transCornersPath);
			s_FileManager.GetPano01Corners().isLoad = true;
			s_FileManager.GetPano02Corners().isLoad = true;

			//====== Following code segments for loading corners as matching points======

			std::ifstream file_0(corner_path01);
			std::ifstream file_1(corner_path02);
			std::ifstream TW_file_0(TW_path01);
			std::ifstream TW_file_1(TW_path02);
			std::string str_0, str_1, str_2, str_3;
			std::vector <glm::vec2> pix_0;
			std::vector <glm::vec2> pix_1;
			std::vector <glm::vec3> xyz_0;
			std::vector <glm::vec3> xyz_1;
			while (std::getline(file_0, str_0))
			{
				std::istringstream iss(str_0);
				glm::vec2 corner_pixel;
				glm::vec3 pos;
				iss >> corner_pixel.x >> corner_pixel.y >> pos.x >> pos.y >> pos.z;
				pix_0.push_back(corner_pixel);
				xyz_0.push_back(pos);

			}
			while (std::getline(TW_file_0, str_1))
			{
				float twTemp;
				std::istringstream iss1(str_1);
				iss1 >> twTemp;
				if (twTemp >= threshold)
				{
					TW_0.push_back(1);
				}
				else
				{
					TW_0.push_back(0);
				}
				//TW_0.push_back(twTemp);
			}
			while (std::getline(file_1, str_2))
			{
				std::istringstream iss(str_2);
				glm::vec2 corner_pixel;
				glm::vec3 pos;
				iss >> corner_pixel.x >> corner_pixel.y >> pos.x >> pos.y >> pos.z;
				pix_1.push_back(corner_pixel);
				xyz_1.push_back(pos);
			}
			while (std::getline(TW_file_1, str_3))
			{
				float twTemp;
				std::istringstream iss1(str_3);
				iss1 >> twTemp;
				if (twTemp >= threshold)
				{
					TW_1.push_back(1);
				}
				else
				{
					TW_1.push_back(0);
				}
				//TW_1.push_back(twTemp);
			}
			buildWall(pano_0_walls, pix_0,xyz_0, TW_0, test);
			buildWall(pano_1_walls, pix_1,xyz_1, TW_1, test);
			start = std::clock();
			elapsed = 0;
			// 轉牆壁
			//po_0 = rollWalls(pano_0_walls);
			//po_1 = rollWalls(pano_1_walls);
			//s_FileManager.SetPano01Info(s_FileManager.GetPano01Filepath(), po_0.first);
			//s_FileManager.SetPano02Info(s_FileManager.GetPano02Filepath(), po_1.first);
			// 列印牆壁資訊
			//for (auto& w : pano_0_walls)
			//{
			//	w.PrintWallInfo();
			//}
			//for (auto& w : pano_1_walls)
			//{
			//	w.PrintWallInfo();
			//}
		}
	}


		// Render input fields
		ImGui::InputInt("Wall 0 Index", &wall_0);
		ImGui::InputInt("Wall 1 Index", &wall_1);
		// Handle input validation (optional)
		if (wall_0 < 0 || wall_0 >= pano_0_walls.size()) {
			ImGui::Text("Invalid Wall 0 Index");
		}
		else if (wall_1 < 0 || wall_1 >= pano_1_walls.size()) {
			ImGui::Text("Invalid Wall 1 Index");
		}
		// 偵測是否有變動
		else if ((wall_0 != prev_wall_0 || wall_1 != prev_wall_1) && wall_0>=0 && wall_1>=0) {
			elapsed = 1000.0 * (std::clock() - start) / CLOCKS_PER_SEC;

			std::filesystem::path path = s_FileManager.GetPano01Filepath().parent_path();
			std::filesystem::path output_file_path = path / (std::to_string(test) + "_" + std::string(user_name.data()) + "_action_info.txt");			std::ofstream output_file(output_file_path);
			populateOnWall(s_MatchPoints, wall_0, wall_1);

			std::vector<MatchPoints> match_points_all;
			match_points_all.push_back(s_MatchPoints);
			std::vector<glm::vec2> errors;

			std::string l_img = (s_FileManager.GetPano01Filepath() / "color.jpg").string();
			std::string r_img = (s_FileManager.GetPano02Filepath() / "color.jpg").string();

			RelativePoseSolver::Solve(l_img.c_str(), r_img.c_str(), match_points_all, errors, 0, 0);

			if (output_file.is_open()) {
				output_file << "PrimaryWall_: " << wall_0 << "\n";
				output_file << "SecondaryWall_: " << wall_1 << "\n";
				output_file << "ElapsedTime_: " << elapsed << "\n";
				output_file << "RotationError_: " << errors[0].x << "\n";
				output_file << "TranslationError_: " << errors[0].y << "\n";
				output_file.close();
				std::cout << "Data saved to " << output_file_path << std::endl;
			}
			else {
				std::cerr << "Failed to open file: " << output_file_path << std::endl;
			}

			// 更新前一次的值
			prev_wall_0 = wall_0;
			prev_wall_1 = wall_1;
		}

		/*
		else if (ImGui::Button("Confirm")) {
			inputs_received = true; // Mark inputs as received
			//std::filesystem::path path = s_FileManager.GetPano01Filepath().parent_path();
			//// save wall_0 wall_1(int) in txt files with file name "pairedWall.txt"
			//std::filesystem::path output_file_path = path / "pairedWall.txt";
			//std::ofstream output_file(output_file_path);
			//if (output_file.is_open()) {
			//	output_file << "PrimaryWall_: " << wall_0 << "\n";
			//	output_file << "SecondaryWall_: " << wall_1 << "\n";
			//	output_file.close();
			//	std::cout << "Data saved to " << output_file_path << std::endl;
			//}
			//else {
			//	std::cerr << "Failed to open file: " << output_file_path << std::endl;
			//}
		}

		if (inputs_received) {
			// Execute pairing logic
			s_MatchPoints.ClearPixel();
			const int order[4] = { 0, 1, 3, 2 };
			pano_0_walls[wall_0].selectWall();
			pano_1_walls[wall_1].selectWall();

			for (auto& i : order)
			{
				const ImU32 col = ImColor(ImVec4((rand() % 256) / 255.0f,
					(rand() % 256) / 255.0f,
					(rand() % 256) / 255.0f, 1.0f));
				s_MatchPoints.AddPoint(
					pano_0_walls[wall_0].pixelCorners[i],
					pano_1_walls[wall_1].pixelCorners[i],
					col, 10,
					pano_0_walls[wall_0].xyzCorners[i],
					pano_1_walls[wall_1].xyzCorners[i]
				);
			}
			PopulateMatching(s_MatchPoints, false);
			inputs_received = false; // Reset for next interaction
		}
		*/
	ImGui::SameLine();
	const char* corner_type[] = { "w/o post", "post", "peak", "LED2Net", "LGT-Net"};
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
	ImGui::Combo("Corner Type", &current_corner_type, corner_type, IM_ARRAYSIZE(corner_type));

	if (ImGui::Button("Rotate")) // Rotate right image corner for correct matching
	{
		s_MatchPoints.RotateRightPixels();
	}
	ImGui::SameLine();
	if (ImGui::Button("Subd"))
	{
		PopulateMatching(s_MatchPoints, false);
	}
	ImGui::SameLine();
	if (ImGui::Button("SubdCircular"))  //circular subdivide
	{
		PopulateMatching(s_MatchPoints, true);
	}

	// Save/Load temporary matching points
	{
		if (ImGui::Button("Save Match points")) // Save current matching points
		{
			std::string name = s_FileManager.GetPano01Filepath().parent_path().string() + "/matching.txt";
			std::ofstream file;
			file.open(name);
			if (file.is_open())
			{
				auto& left = s_MatchPoints.left_pixels;
				auto& right = s_MatchPoints.right_pixels;
				auto& weights = s_MatchPoints.weights;
				auto& left_positions = s_MatchPoints.left_positions;
				auto& right_positions = s_MatchPoints.right_positions;
				for (int i = 0; i < s_MatchPoints.size(); ++i)
				{
					file << left[i].x << " " << left[i].y << " " << right[i].x << " " << right[i].y << " " << weights[i] << " " << left_positions[i].x << " " << left_positions[i].y << " " << left_positions[i].z << " " << right_positions[i].x << " " << right_positions[i].y << " " << right_positions[i].z << "\n";
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Load Match points")) // Load matching points from txt file
		{
			s_MatchPoints.ClearPixel(); //  clear before load
			std::string name = s_FileManager.GetPano01Filepath().parent_path().string() + "/matching.txt";
			std::ifstream file;
			file.open(name);
			if (file.is_open())
			{
				std::string str;
				glm::vec2 left, right;
				uint32_t weight;
				glm::vec3 left_pos, right_pos;
				while (std::getline(file, str))
				{
					std::istringstream iss(str);
					iss >> left.x >> left.y >> right.x >> right.y >> weight >> left_pos.x >> left_pos.y >> left_pos.z >> right_pos.x >> right_pos.y >> right_pos.z;
					const ImU32 col = ImColor(ImVec4((rand() % 256) / 255.0f, (rand() % 256) / 255.0f, (rand() % 256) / 255.0f, 1.0f));
					
					s_MatchPoints.AddPoint(left, right, col, weight, left_pos, right_pos);
				}
			}
			else
			{
				std::cout << "There is no matching file " << std::quoted(name) << std::endl;
			}
		}
	}
	ImGui::Separator();

	std::string left_img = s_FileManager.GetPano01Filepath().string() + "/color.jpg";
	std::string right_img = s_FileManager.GetPano02Filepath().string() + "/color.jpg";
	static int method_type = 0; //0: 8-point algorithm, 1:Gurobi

	const char* solve_methods[] = { "8-point", "Gurobi" };
	if (ImGui::Button("Calculate"))
	{
		std::vector<MatchPoints> match_points_all;
		match_points_all.push_back(s_MatchPoints);
		std::vector<glm::vec2> errors;
		RelativePoseSolver::Solve(left_img.c_str(), right_img.c_str(), match_points_all, errors, method_type,corRot);
	}
	ImGui::SameLine();
	if (ImGui::Button("TryAll"))
	{
		s_TextLog.Clear();
		//let's try all possible wall-wall matching and report the best one (w.r.t. gt pose)
		glm::vec2 best_error(-1, -1);
		TryAll(left_img, right_img, method_type, best_error);
	}
	ImGui::SameLine();
	if (ImGui::Button("Eval"))
	{
		eval();
	}
	ImGui::SameLine();
	if (ImGui::Button("SIFT")) // SIFT Solver
	{
		s_MatchPoints.ClearPixel();
		SIFTSolver::Solve(left_img.c_str(), right_img.c_str(), m_PanoPos_gt, s_MatchPoints);
	}
	ImGui::SameLine();
	if (ImGui::Button("LoFTR")) // LoFTR Solver
	{
		s_MatchPoints.ClearPixel();
		std::string loftr_file = s_FileManager.GetPano01Filepath().parent_path().string() + "/loftr.txt";
		std::cout << loftr_file << std::endl;
		LoFTRSolver::Solve(left_img.c_str(), right_img.c_str(), loftr_file, s_MatchPoints);
	}
	ImGui::SameLine();
	if (ImGui::Button("SPHORB")) // SPHORB Solver
	{
		s_MatchPoints.ClearPixel();
		SPHORBSolver::Solve(left_img.c_str(), right_img.c_str(), s_MatchPoints);
	}
	ImGui::Separator();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
	ImGui::Combo("Solve Method", &method_type, solve_methods, IM_ARRAYSIZE(solve_methods));

	//texts_to_show:
	// list all result (Rotation / Translation) and highlight best error as green color!
	ImGui::Separator();
	s_TextLog.Draw();
	
	/*{
		const int num_lines_to_show = 8;
		int lines_to_show = std::min((int)g_texts_to_show.size(), num_lines_to_show);
		for (int i = lines_to_show; i > 0; i--)
		{
			char str[1000] = { NULL };
			sprintf(str, "[%03d] %s", g_texts_to_show.size()-i, g_texts_to_show[g_texts_to_show.size() - i].c_str());
			ImGui::Text(str);
		}
	}*/
}


bool ToolLayer::eval()
{
	int corner_type = 1;
	const char* corner_filename[] = { "pred_corner_raw.txt", "pred_corner.txt", "pred_corner_peak.txt", "pred_corner_LED2Net.txt", "pred_corner_LGT.txt" };
	const char* data_dir = "D:\\noah\\PanoramicCameraPose\\PanoramicCameraPose\\assets\\test_data\\100_testing_set";

	for (const auto& score_entry : std::filesystem::directory_iterator(data_dir))
	{
		for (const auto& id_entry : std::filesystem::directory_iterator(score_entry))
		{
			std::wstring filepath = id_entry.path();
			std::wstring p1, p2;
			get_paired_pano_path(filepath, p1, p2);

			if (!filepath.empty())
			{
				s_FileManager.SetPano01Info(p1);
				s_FileManager.SetPano02Info(p2);
				s_MatchPoints.ClearPixel();
				pano_0_walls.clear();
				pano_1_walls.clear();
			}

			std::filesystem::path corner_path01 = s_FileManager.GetPano01Filepath() / corner_filename[corner_type];
			std::filesystem::path corner_path02 = s_FileManager.GetPano02Filepath() / corner_filename[corner_type];
			std::filesystem::path TW_path01 = s_FileManager.GetPano01Filepath() / "TW.txt";
			std::filesystem::path TW_path02 = s_FileManager.GetPano02Filepath() / "TW.txt";
			std::filesystem::path transCornersPath = s_FileManager.GetPano02Filepath().parent_path() / "trans_corner.txt";

			if (corner_path01.empty() || corner_path02.empty() || TW_path01.empty() || TW_path02.empty())
			{
				std::cout << "Please Upload " << corner_filename[corner_type] << std::endl;
			}
			else if (transCornersPath.empty())
			{
				std::cout << "Please Upload trans_corner.txt" << std::endl;
			}

			std::vector<float> TW_0, TW_1;
			float threshold = 0.7f;

			std::vector<glm::vec2> pix_0, pix_1;
			std::vector<glm::vec3> xyz_0, xyz_1;

			s_FileManager.SetPano01Corners(corner_path01);
			s_FileManager.SetPano02Corners(corner_path02);
			s_FileManager.SetTransformCorners(transCornersPath);

			std::ifstream file_0(corner_path01);
			std::ifstream file_1(corner_path02);
			std::ifstream TW_file_0(TW_path01);
			std::ifstream TW_file_1(TW_path02);

			std::string line;
			while (std::getline(file_0, line))
			{
				std::istringstream iss(line);
				glm::vec2 corner_pixel;
				glm::vec3 pos;
				iss >> corner_pixel.x >> corner_pixel.y >> pos.x >> pos.y >> pos.z;
				pix_0.push_back(corner_pixel);
				xyz_0.push_back(pos);
			}
			while (std::getline(TW_file_0, line))
			{
				float twTemp;
				std::istringstream iss(line);
				iss >> twTemp;
				TW_0.push_back(twTemp >= threshold ? 1.0f : 0.0f);
			}
			while (std::getline(file_1, line))
			{
				std::istringstream iss(line);
				glm::vec2 corner_pixel;
				glm::vec3 pos;
				iss >> corner_pixel.x >> corner_pixel.y >> pos.x >> pos.y >> pos.z;
				pix_1.push_back(corner_pixel);
				xyz_1.push_back(pos);
			}
			while (std::getline(TW_file_1, line))
			{
				float twTemp;
				std::istringstream iss(line);
				iss >> twTemp;
				TW_1.push_back(twTemp >= threshold ? 1.0f : 0.0f);
			}

			buildWall(pano_0_walls, pix_0, xyz_0, TW_0, false);
			buildWall(pano_1_walls, pix_1, xyz_1, TW_1, false);

			float best_error = INFINITY;
			std::pair<int, int> best_pair(-1, -1);
			std::pair<float, float> best_pair_error(-1, -1);


			for (size_t i = 0; i < pano_0_walls.size(); ++i)
			{
				for (size_t j = 0; j < pano_1_walls.size(); ++j)
				{
					populateOnWall(s_MatchPoints, i, j);

					std::vector<MatchPoints> match_points_all;
					match_points_all.push_back(s_MatchPoints);
					std::vector<glm::vec2> errors;

					std::string l_img = (s_FileManager.GetPano01Filepath() / "color.jpg").string();
					std::string r_img = (s_FileManager.GetPano02Filepath() / "color.jpg").string();

					RelativePoseSolver::Solve(l_img.c_str(), r_img.c_str(), match_points_all, errors, 0, 0);

					float error_sum = errors[0].x + errors[0].y;
					if (error_sum < best_error)
					{
						best_error = error_sum;
						best_pair_error = { errors[0].x,errors[0].y };
						best_pair = { static_cast<int>(i), static_cast<int>(j) };
					}
				}
			}

			// Save the best error and best pair to a file
			std::wstring out = filepath + L"/best_result.txt";
			std::ofstream output_file(out);
			if (output_file.is_open())
			{
				//output_file << "Best Error: " << best_error << "\n";
				output_file << "Rotation, Translation: (" << best_pair_error.first << ", " << best_pair_error.second << ")\n";
				output_file << "Best Pair: (" << best_pair.first << ", " << best_pair.second << ")\n";
				output_file.close();
			}
		}
	}
	std::cout << "eval end" << std::endl;

	return true;
}


bool ToolLayer::TryAll(std::string &left_img, std::string &right_img, int method_type, glm::vec2& best_error)
{
	//assume corners are loaded
	CornerPoints &corners1 = s_FileManager.GetPano01Corners();
	CornerPoints &corners2 = s_FileManager.GetPano02Corners();

	const ImU32 col = ImColor(0);

	//let's collect all possible ways of matching
	std::vector<MatchPoints> match_points_all;
	std::vector<std::pair<int, int>> match_walls;

	//for every wall of first panorama, try match to every wall of second panorama
	for (int ii = 0; ii < corners1.ceil_pixels.size(); ii++)
	{
		int c10 = ii;
		int c11 = (ii + 1) % corners1.ceil_pixels.size();

		for (int jj = 0; jj < corners2.ceil_pixels.size(); jj++)
		{
			int c20 = jj;
			int c21 = (jj + 1) % corners2.ceil_pixels.size();

			//temporarily generate a MatchPoints for this wall-wall matching
			MatchPoints match_points;
			//4 corners of a wall:
			match_points.AddPoint(corners1.ceil_pixels[c10], corners2.ceil_pixels[c20],
				col, 10, corners1.ceil_positions[c10], corners2.ceil_positions[c20]);
			match_points.AddPoint(corners1.ceil_pixels[c11], corners2.ceil_pixels[c21],
				col, 10, corners1.ceil_positions[c11], corners2.ceil_positions[c21]);
			match_points.AddPoint(corners1.floor_pixels[c11], corners2.floor_pixels[c21],
				col, 10, corners1.floor_positions[c11], corners2.floor_positions[c21]);
			match_points.AddPoint(corners1.floor_pixels[c10], corners2.floor_pixels[c20],
				col, 10, corners1.floor_positions[c10], corners2.floor_positions[c20]);

			PopulateMatching(match_points, false);

			//then save the populated MatchPoints
			match_points_all.push_back(match_points);
			match_walls.emplace_back(ii, jj);
		}
	}

	std::vector<glm::vec2> errors;
	RelativePoseSolver::Solve(left_img.c_str(), right_img.c_str(), match_points_all, errors, method_type,corRot);

	//report errors
	std::stringstream buffer;
	best_error = glm::vec2(999);
	std::cout << "match_walls size():" << match_walls.size() << std::endl;
	std::cout << "errors.size():" << errors.size() << std::endl;

	// sort error by index vector
	std::vector<std::size_t> p(errors.size());
	std::iota(p.begin(), p.end(), 0);
	std::sort(p.begin(), p.end(), [&](std::size_t i, std::size_t j) {
		return glm::length(errors[i]) < glm::length(errors[j]);
	});

	for (int i = 0; i < errors.size(); i++)
	{
		buffer.str(std::string());
		buffer << "wall " << match_walls[p[i]].first << " <-> Wall " << match_walls[p[i]].second << " err#" << i << ": " << "RE = " << errors[p[i]].x << ", TE = " << errors[p[i]].y;
		s_TextLog.AddLog("%s\n", buffer.str().c_str());
		s_TextLog.best_line = 0;
		best_error = errors[p[0]];
		//find the smallest length (sum) of errors
		/*if (errors[i].x >= 0 && glm::length(errors[i]) < glm::length(best_error))
		{
			s_TextLog.best_line = i;
			best_error = errors[i];
		}*/
	}
	std::cout << "best_err:" << best_error.x << "," << best_error.y << std::endl;

	// two best wall result
	MatchPoints& match_points = match_points_all[p[0]];
	std::cout << "left:" << match_points.left_pixels.size() << "\tright:" << match_points.right_pixels.size() << std::endl;


	int cur_wall01 = match_walls[p[0]].first;
	int i = 1;
	while (i < p.size() && match_walls[p[i]].first == cur_wall01) ++i;
	
	MatchPoints& pts = match_points_all[p[i]];
	std::cout << "left:" << pts.left_pixels.size() << "\tright:" << pts.right_pixels.size() << std::endl;

	match_points.left_pixels.insert(match_points.left_pixels.end(), pts.left_pixels.begin(), pts.left_pixels.end());
	match_points.right_pixels.insert(match_points.right_pixels.end(), pts.right_pixels.begin(), pts.right_pixels.end());
	std::vector<MatchPoints> temp = { match_points };
	RelativePoseSolver::Solve(left_img.c_str(), right_img.c_str(), temp, errors, method_type,corRot);

	
	std::cout << "temp size():" << temp.size() << std::endl;
	std::cout << "left:" << temp[0].left_pixels.size() << "\tright:" << temp[0].right_pixels.size() << std::endl;
	std::cout << "errors.size():" << errors.size() << std::endl;

	buffer.str(std::string());
	buffer << "Wall " << match_walls[p[0]].first << " <-> Wall " << match_walls[p[0]].second;
	buffer << " Wall " << match_walls[p[i]].first << " <-> Wall " << match_walls[p[i]].second;
	buffer << " RE = " << errors[0].x << " TE = " << errors[0].y << std::endl;
	s_TextLog.AddLog("%s\n", buffer.str().c_str());
	if (errors[0].x >= 0 && glm::length(errors[0]) < glm::length(best_error))
	{
		s_TextLog.best_line = std::size(p);
		best_error = errors[0];
	}
	return true;
}

// static member initialization
MatchPoints ToolLayer::s_MatchPoints;
FileManager ToolLayer::s_FileManager;
Log ToolLayer::s_TextLog;