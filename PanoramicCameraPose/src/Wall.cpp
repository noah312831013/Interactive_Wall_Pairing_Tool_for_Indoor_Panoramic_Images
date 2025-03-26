#include "pch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Wall.h"
#include "FileManager.h"

void buildWall(std::vector<Wall>& Walls, std::vector<glm::vec2> pix,std::vector<glm::vec3> xyz, std::vector<float> TW,const bool test) 
{
    Walls.clear();
    // auto [r_pix,r_xyz] = rollVectorForTwoParts(pix,xyz);
    //std::vector<glm::vec3> xyz = pix2xyz(pix,1024);
    int num_points = pix.size() / 2;  // Since we have equal ceiling and floor points
    if (num_points != TW.size() || pix.size()!=xyz.size())
    {
        std::cout << "length error" << std::endl;
        std::cout << "num_points: " << num_points << std::endl;
        std::cout << "TW_points: " << TW.size() << std::endl;
        std::cout << "pix_points: " << pix.size() << std::endl;
        std::cout << "xyz_points: " << xyz.size() << std::endl;
        return;
    }
    for (int i = 0; i < num_points; ++i) {
        // Indices for ceiling and floor
        int ceil_idx_0 = i;
        int ceil_idx_1 = (i + 1) % num_points;
        int floor_idx_0 = i + num_points;
        int floor_idx_1 = (i + 1) % num_points + num_points;

        // Prepare the pixel and xyz corners for the current wall
        glm::vec2 pixelCorners[4] = {
            pix[ceil_idx_1], // ceil_right
            pix[ceil_idx_0], // ceil_left
            pix[floor_idx_1], // floor_right
            pix[floor_idx_0]  // floor_left
        };

        glm::vec3 xyzCorners[4] = {
            xyz[ceil_idx_1], // ceil_right
            xyz[ceil_idx_0], // ceil_left
            xyz[floor_idx_1], // floor_right
            xyz[floor_idx_0]  // floor_left
        };

        // Wall triviality check
        float isTrivial = TW[i];
        Wall wall(pixelCorners, xyzCorners, &isTrivial, &i,&test);

        // Add wall to the result vector
        Walls.push_back(wall);
    }
    if (Walls.size() != num_points)
    {
        std::cout << "walls length error" << std::endl;
        Walls.clear();
        return;
    }
}

// Function to roll a vector by 'n' positions
template <typename VecType>
void rollVector(std::vector<VecType>& vec, int roll_steps) {
    if (vec.empty() || roll_steps == 0) return;

    // Normalize roll_steps to ensure it is within bounds
    roll_steps = roll_steps % vec.size();
    if (roll_steps < 0) roll_steps += vec.size();

    // Rotate the vector by roll_steps
    std::rotate(vec.begin(), vec.begin() + roll_steps, vec.end());
}

// Function to split, roll and merge the vector based on the minimum x in the first half
std::pair<std::vector<glm::vec2>, std::vector<glm::vec3>> rollVectorForTwoParts(
    std::vector<glm::vec2>& input,
    std::vector<glm::vec3>& input2)
{
    size_t n = input.size();
    size_t n2 = input2.size();

    if (n == 0 || n2 == 0) return { {}, {} }; // Handle empty vectors

    // Split the first vector into two halves
    size_t mid = n / 2;
    std::vector<glm::vec2> first_half(input.begin(), input.begin() + mid);
    std::vector<glm::vec2> second_half(input.begin() + mid, input.end());

    // Split the second vector into two halves
    std::vector<glm::vec3> first_half2(input2.begin(), input2.begin() + mid);
    std::vector<glm::vec3> second_half2(input2.begin() + mid, input2.end());

    // Find the element with the minimum x value in the first half
    auto min_x_iter = std::min_element(first_half.begin(), first_half.end(), [](const glm::vec2& a, const glm::vec2& b) {
        return a.x < b.x;
        });

    // Calculate the roll steps for the first half
    int roll_steps_first_half = std::distance(first_half.begin(), min_x_iter);

    // Roll both halves
    rollVector(first_half, roll_steps_first_half);
    rollVector(second_half, roll_steps_first_half);
    rollVector(first_half2, roll_steps_first_half);
    rollVector(second_half2, roll_steps_first_half);

    // Merge the two halves back
    std::vector<glm::vec2> result;
    std::vector<glm::vec3> result2;

    result.reserve(n);
    result2.reserve(n2);

    result.insert(result.end(), first_half.begin(), first_half.end());
    result.insert(result.end(), second_half.begin(), second_half.end());
    result2.insert(result2.end(), first_half2.begin(), first_half2.end());
    result2.insert(result2.end(), second_half2.begin(), second_half2.end());

    // Return the results as a pair
    return { result, result2 };
}
//pixelOffset and degree
std::pair<int,float> rollWalls(std::vector<Wall>& walls)
{
    int l = walls.size();
    int id = -1;
    if (l < 1) return {-1,-1};

    // Check if any wall is not trivial
    bool needRoll = false;
    for (auto& wall : walls) {
        if (!wall.isTrivial) {
            id = wall.id;
            needRoll = true;
            break;
        }
    }
    if (!needRoll) return {-1,-1};
    if (walls[l - 1].isTrivial)
    {
        // Compute pixel offset and angle
        const float width = 1023.0f;
        float pixelOffset = width - walls[id].pixelCorners[0].x -
            (walls[id].pixelCorners[1].x - walls[id].pixelCorners[0].x) / 2;
        float radian = (pixelOffset / width * 2 * glm::pi<float>());
        float degrees = radian * (180.0f / glm::pi<float>());
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), radian, glm::vec3(0.0f, 0.0f, 1.0f));

        // Normalization lambda
        auto normalize = [](float value, float range) {
            value = std::fmod(value, range);
            return value < 0 ? value + range : value;
            };

        // Update walls
        for (auto& wall : walls) {
            //for (int i = 0; i < 4; ++i) {
            //    wall.pixelCorners[i].x = normalize(wall.pixelCorners[i].x + pixelOffset, 1024.0f);

            //    glm::vec4 transformed = rotationMatrix * glm::vec4(wall.xyzCorners[i], 1.0f);
            //    wall.xyzCorners[i] = glm::vec3(transformed);
            //}
            wall.selectionCenters.x = normalize(wall.selectionCenters.x + pixelOffset, 1024.0f);

            for (auto& pt : wall.slerped_ceil_pix)
            {
                pt.x = normalize(pt.x+pixelOffset, 1024.f);
            }
            for (auto& pt : wall.slerped_floor_pix)
            {
                pt.x = normalize(pt.x+pixelOffset, 1024.f);
            }

        }

        return { pixelOffset, degrees};
    }
}