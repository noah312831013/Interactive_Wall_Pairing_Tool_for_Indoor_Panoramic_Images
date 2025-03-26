#pragma once
#include "imgui/imgui.h"
#include "mathStuff.h"
#include <cmath>
#include "FileManager.h"

static glm::vec2 PosToSpherical_1(glm::vec3 p, bool flip_x = true)
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
static glm::vec2 SphericalToXY_1(glm::vec2 s, float width, float height)
{
    glm::vec2 xy;
    xy.x = s.x / (2 * glm::pi<float>()) * width;
    xy.y = s.y / (glm::pi<float>()) * height;

    return xy;
}

struct Wall {
    glm::vec2 pixelCorners[4]; //ceil_left, ceil_right, floor_left, floor_right
    glm::vec3 xyzCorners[4];
    glm::vec2 selectionCenters;
    float isTrivial = 0.0;
    int id = -1;
    ImU32 col= IM_COL32_WHITE;
    ImU32 col_text_bg = IM_COL32(0, 0, 0, 125);
    std::vector<ImVec2> slerped_ceil_pix;
    std::vector<ImVec2> slerped_floor_pix;
    const int sample = 30;

    ImVec2 text_size = ImGui::CalcTextSize(std::to_string(id).c_str());
    float radius = std::max(text_size.x, text_size.y) * 0.6f;

    Wall(const glm::vec2(&inputPixelCorners)[4], const glm::vec3(&inputXyzCorners)[4], const float* TW,const int* iid, const bool* test) {
        // 使用 std::copy 進行初始化
        std::copy(std::begin(inputPixelCorners), std::end(inputPixelCorners), std::begin(pixelCorners));
        std::copy(std::begin(inputXyzCorners), std::end(inputXyzCorners), std::begin(xyzCorners));
        id = *iid;
        // 計算 selection center
        //if (!*islast)
        //{
        //    selectionCenters.x = (pixelCorners[0].x + pixelCorners[1].x) / 2.0f;
        //    selectionCenters.y = (pixelCorners[0].y + pixelCorners[2].y) / 2.0f;
        //}
        //else
        //{
        //    selectionCenters.x = static_cast<float>(static_cast<int>((pixelCorners[0].x + pixelCorners[1].x + 1024.0f) / 2.0f) % 1024);
        //    selectionCenters.y = (pixelCorners[0].y + pixelCorners[2].y) / 2.0f;
        //}
        glm::vec3 center = (xyzCorners[0] + xyzCorners[1] + xyzCorners[2] + xyzCorners[3]) / glm::vec3(4.0f);
        glm::vec2 pix_center = SphericalToXY_1(PosToSpherical_1(center,false),1024,512);
        //std::vector<glm::vec3> temp;
        //temp.push_back(center);
        //glm::vec2 pix_center = xyz2pix(temp,1024)[0];
        selectionCenters.x = pix_center.x;
        selectionCenters.y = pix_center.y;


        // 初始化屬性
        isTrivial = *TW;
        if(*test && !isTrivial)
        {
            col = IM_COL32(0, 0, 0, 0); // Full opacity
            col_text_bg = IM_COL32(0, 0, 0, 0);
        }
        

        // 進行插值計算並轉換為 ImVec2
        auto ceil_points = InterpolateOnUnitSphere(xyzCorners[0], xyzCorners[1], sample);
        auto floor_points = InterpolateOnUnitSphere(xyzCorners[2], xyzCorners[3], sample);

        slerped_ceil_pix = ConvertToImVec2(SphericalToXYBatch(PosToSphericalBatch(ceil_points,false),1024,512));
        slerped_floor_pix = ConvertToImVec2(SphericalToXYBatch(PosToSphericalBatch(floor_points,false),1024,512));
    }
    void PrintWallInfo() const {
        std::cout << "Wall ID: " << id << "\n";
        //std::cout << (isTrivial ? "Trivial Case" : "Non-Trivial Case") << "\n";
        std::cout << "Trivility"<< isTrivial << "\n";
        std::cout << "Pixel Corners (ceil_left, ceil_right, floor_left, floor_right):\n";
        for (int i = 0; i < 4; ++i) {
            std::cout << "  Pixel Corner [" << i << "]: ("
                << pixelCorners[i].x << ", " << pixelCorners[i].y << ")\n";
        }
        std::cout << "XYZ Corners (ceil_left, ceil_right, floor_left, floor_right):\n";
        for (int i = 0; i < 4; ++i) {
            std::cout << "  XYZ Corner [" << i << "]: ("
                << xyzCorners[i].x << ", " << xyzCorners[i].y << ", " << xyzCorners[i].z << ")\n";
        }
        std::cout << "center\n";
        std::cout << selectionCenters.x << "," << selectionCenters.y << std::endl;
        std::cout << std::endl;
    }
    void RenderWall(ImDrawList* draw_list, ImVec2 viewport_bound, float m_ratio, bool isSecondPano) {
        const float width_half = 200.0f * m_ratio;
        const float pano_offset = (isSecondPano * 512) * m_ratio;
        const float offset_x = viewport_bound.x;
        const float offset_y = pano_offset + viewport_bound.y;
        const int n = sample - 1;
        // Convert glm::vec2 to ImVec2 for rendering the ID
        ImVec2 id_position = ImVec2(selectionCenters.x * m_ratio + offset_x, selectionCenters.y * m_ratio + offset_y);
        ImVec2 circle_center = ImVec2(id_position.x + text_size.x / 2, id_position.y + text_size.y / 2);

        // Render the wall boundaries
        for (int i = 0; i < n; ++i) {
            ImVec2 ceil_start = {
                slerped_ceil_pix[i].x * m_ratio + offset_x,
                slerped_ceil_pix[i].y * m_ratio + offset_y
            };
            ImVec2 ceil_end = {
                slerped_ceil_pix[i + 1].x * m_ratio + offset_x,
                slerped_ceil_pix[i + 1].y * m_ratio + offset_y
            };
            ImVec2 floor_start = {
                slerped_floor_pix[i].x * m_ratio + offset_x,
                slerped_floor_pix[i].y * m_ratio + offset_y
            };
            ImVec2 floor_end = {
                slerped_floor_pix[i + 1].x * m_ratio + offset_x,
                slerped_floor_pix[i + 1].y * m_ratio + offset_y
            };

            // Avoid crossing image boundary
            if (std::abs(ceil_start.x - ceil_end.x) < width_half) {
                draw_list->AddLine(ceil_start, ceil_end, col, 2.0f);
            }
            if (std::abs(floor_start.x - floor_end.x) < width_half) {
                draw_list->AddLine(floor_start, floor_end, col, 2.0f);
            }
            
        }

        // Draw connecting lines between the front and back points
        ImVec2 ceil_front = {
            slerped_ceil_pix.front().x * m_ratio + offset_x,
            slerped_ceil_pix.front().y * m_ratio + offset_y
        };
        ImVec2 ceil_back = {
            slerped_ceil_pix.back().x * m_ratio + offset_x,
            slerped_ceil_pix.back().y * m_ratio + offset_y
        };
        ImVec2 floor_front = {
            slerped_floor_pix.front().x * m_ratio + offset_x,
            slerped_floor_pix.front().y * m_ratio + offset_y
        };
        ImVec2 floor_back = {
            slerped_floor_pix.back().x * m_ratio + offset_x,
            slerped_floor_pix.back().y * m_ratio + offset_y
        };

        draw_list->AddLine(ceil_front, floor_front, col, 2.0f);
        draw_list->AddLine(ceil_back, floor_back, col, 2.0f);



        // 畫黑色圓形背景
        draw_list->AddCircleFilled(circle_center, radius, col_text_bg);

        // 畫 ID 文字
        draw_list->AddText(id_position, col, std::to_string(id).c_str());
    }
    void selectWall()
    {
        col = IM_COL32(0, 0, 255, 255);
    }
    //void setWallCol()
    //{
    //    col = isTrivial ? IM_COL32_WHITE : IM_COL32_RED;
    //}
    void setWallCol()
    {
        // Ensure isTrivial is between 0 and 1
        float clampedTrivial = std::clamp(isTrivial, 0.0f, 1.0f);

        // Interpolate between red and white based on clampedTrivial
        // IM_COL32 is a macro that takes RGBA values (0-255)
        int r = static_cast<int>(255 * (1 - clampedTrivial) + 255 * clampedTrivial); // Red channel
        int g = static_cast<int>(0 * (1 - clampedTrivial) + 255 * clampedTrivial);   // Green channel
        int b = static_cast<int>(0 * (1 - clampedTrivial) + 255 * clampedTrivial);   // Blue channel

        col = IM_COL32(r, g, b, 255); // Full opacity
    }
};

void buildWall(std::vector<Wall>& Walls, std::vector<glm::vec2> pix,std::vector<glm::vec3> xyz, std::vector<float> TW, const bool test);
template <typename VecType>
void rollVector(std::vector<VecType>& vec, int roll_steps);
std::pair<std::vector<glm::vec2>, std::vector<glm::vec3>> rollVectorForTwoParts(std::vector<glm::vec2>& input, std::vector<glm::vec3>& input2);
std::pair <int,float> rollWalls(std::vector<Wall>& walls);