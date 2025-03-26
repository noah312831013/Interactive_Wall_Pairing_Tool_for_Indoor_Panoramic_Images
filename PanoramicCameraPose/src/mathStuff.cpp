#include "pch.h"
#include "mathStuff.h"

std::vector<glm::vec2> PosToSphericalBatch(const std::vector<glm::vec3>& points, bool flip_x = true) {
    std::vector<glm::vec2> result;
    result.reserve(points.size());

    for (const auto& p : points) {
        glm::vec3 normalized_p = glm::normalize(p);
        glm::vec3 processed_p = normalized_p;

        if (flip_x) {
            processed_p.x = -processed_p.x;
        }
        else {
            std::swap(processed_p.x, processed_p.y);
            processed_p.y = -processed_p.y;
        }

        glm::vec2 c;
        c.y = acos(processed_p.z);  // Zenith

        // Azimuth
        if (processed_p.x > 0) {
            c.x = atan(processed_p.y / processed_p.x);
        }
        else if (processed_p.x < 0 && processed_p.y >= 0) {
            c.x = atan(processed_p.y / processed_p.x) + glm::pi<float>();
        }
        else if (processed_p.x < 0 && processed_p.y < 0) {
            c.x = atan(processed_p.y / processed_p.x) - glm::pi<float>();
        }
        else if (processed_p.x == 0 && processed_p.y > 0) {
            c.x = glm::pi<float>() / 2;
        }
        else if (processed_p.x == 0 && processed_p.y < 0) {
            c.x = -glm::pi<float>() / 2;
        }
        else {
            std::cout << "[PosToSphericalBatch] azimuth undefined!" << std::endl;
            c.x = 0;
        }

        // Ensure azimuth is not negative
        if (c.x < 0) {
            c.x = 2 * glm::pi<float>() + c.x;
        }

        result.push_back(c);
    }

    return result;
}

// Batch version of SphericalToXY
std::vector<glm::vec2> SphericalToXYBatch(const std::vector<glm::vec2>& spherical_coords, float width, float height) {
    std::vector<glm::vec2> result;
    result.reserve(spherical_coords.size());

    for (const auto& s : spherical_coords) {
        glm::vec2 xy;
        xy.x = s.x / (2 * glm::pi<float>()) * width;
        xy.y = s.y / glm::pi<float>() * height;
        result.push_back(xy);
    }

    return result;
}


std::vector<ImVec2> ConvertToImVec2(const std::vector<glm::vec2>& glmVecs) {
    std::vector<ImVec2> imVecs;
    imVecs.reserve(glmVecs.size()); // Reserve space to avoid reallocations

    for (const auto& glmVec : glmVecs) {
        imVecs.emplace_back(glmVec.x, glmVec.y);
    }

    return imVecs;
}
std::vector<glm::vec3> InterpolateOnUnitSphere(const glm::vec3& start, const glm::vec3& end, int sampleCount) {
    std::vector<glm::vec3> result;

    // 確保樣本數大於等於2，至少包含頭尾
    if (sampleCount < 2) sampleCount = 2;

    // 歸一化輸入向量
    glm::vec3 startNorm = glm::normalize(start);
    glm::vec3 endNorm = glm::normalize(end);

    // 計算兩個向量之間的角度
    float dotProduct = glm::clamp(glm::dot(startNorm, endNorm), -1.0f, 1.0f);
    float angle = std::acos(dotProduct);

    // 將起始向量加入結果
    result.push_back(startNorm);

    // 插值樣本
    for (int i = 1; i < sampleCount - 1; ++i) {
        // 計算插值參數 t
        float t = static_cast<float>(i) / (sampleCount - 1);

        // 使用球面插值 (Slerp)
        float sinAngle = std::sin(angle);
        float weightA = std::sin((1 - t) * angle) / sinAngle;
        float weightB = std::sin(t * angle) / sinAngle;

        // 計算插值向量
        glm::vec3 interpolated = glm::normalize(weightA * startNorm + weightB * endNorm);
        result.push_back(interpolated);
    }

    // 將結束向量加入結果
    result.push_back(endNorm);

    return result;
}

// pix2uv
std::vector<glm::vec2> pix2uv(const std::vector<glm::vec2>& xy, int width) {
    float h = width / 2.0f;
    glm::vec2 div = glm::vec2(static_cast<float>(width), h);
    std::vector<glm::vec2> result;
    result.reserve(xy.size());

    for (const auto& point : xy) {
        glm::vec2 uv = point / div;
        result.push_back(uv);
    }

    return result;
}

// uv2spherical
std::vector<glm::vec2> uv2spherical(const std::vector<glm::vec2>& uv) {
    std::vector<glm::vec2> result;
    result.reserve(uv.size());

    for (const auto& point : uv) {
        float X = point.x * 2.0f * glm::pi<float>() - glm::pi<float>();
        float Y = point.y * glm::pi<float>() - glm::half_pi<float>();
        result.push_back(glm::vec2(X, Y));
    }

    return result;
}

// spherical2xyz
std::vector<glm::vec3> spherical2xyz(const std::vector<glm::vec2>& spherical) {
    std::vector<glm::vec3> result;
    result.reserve(spherical.size());

    for (const auto& point : spherical) {
        float lon = point.x;
        float lat = point.y;
        float x = glm::cos(lat) * glm::sin(lon);
        float y = glm::sin(lat);
        float z = glm::cos(lat) * glm::cos(lon);
        result.push_back(glm::vec3(x, y, z));
    }

    return result;
}

// pix2xyz
std::vector<glm::vec3> pix2xyz(const std::vector<glm::vec2>& xy, int width) {
    auto spherical = uv2spherical(pix2uv(xy, width));
    return spherical2xyz(spherical);
}

// spherical2pix
std::vector<glm::vec2> spherical2pix(const std::vector<glm::vec2>& spherical, int width) {
    std::vector<glm::vec2> result;
    result.reserve(spherical.size());

    for (const auto& point : spherical) {
        float X = (point.x + glm::pi<float>()) / (2.0f * glm::pi<float>());
        float Y = (point.y + glm::half_pi<float>()) / glm::pi<float>();
        glm::vec2 uv = glm::vec2(X, Y);
        glm::vec2 div = glm::vec2(static_cast<float>(width), width / 2.0f);
        glm::vec2 xy = uv * div;
        result.push_back(xy);
    }

    return result;
}

// xyz2spherical
std::vector<glm::vec2> xyz2spherical(const std::vector<glm::vec3>& xyz) {
    std::vector<glm::vec2> spherical;
    spherical.reserve(xyz.size());

    for (const auto& pos : xyz) {
        float theta = std::atan2(pos.x, pos.z);
        float phi = std::asin(pos.y);
        spherical.push_back(glm::vec2(theta, phi));
    }

    return spherical;
}

// xyz2pix
std::vector<glm::vec2> xyz2pix(const std::vector<glm::vec3>& xyz, int width) {
    auto spherical = xyz2spherical(xyz);
    auto pix = spherical2pix(spherical, width);
    return pix;
}