#include "pch.h"
#include "Application.h"
#include "PanoLayer.h"
#include "ToolLayer.h"
#include "imgui/imgui.h"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


static GLuint aim_tex = 0;
static int aim_w = 0;
static int aim_h = 0;
extern std::pair <int, float> po_0;
extern std::pair <int, float> po_1;

//----------------Temporary functions for rendering imgui drawlist to texture----------------
// Make FB
int twice = 2;
ImVec2 m_fboSize{ 512.0f, 1024.0f };
GLuint fbo, tex;

// Creates  framebuffer / textures
void makeFBO(ImVec2 size, GLuint* fbo, GLuint* tex) {
    glDeleteFramebuffers(1, fbo);
    GLuint color = 0;
    GLuint depth = 0;
    glGenFramebuffers(1, fbo);
    glGenTextures(1, &color);
    glGenRenderbuffers(1, &depth);

    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    *tex = color;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Makes draw data for renderer
ImDrawData makeDrawData(ImDrawList** dl, ImVec2 pos, ImVec2 size) {
    ImDrawData draw_data = ImDrawData();

    draw_data.Valid = true;
    draw_data.CmdLists = dl;
    draw_data.CmdListsCount = 1;
    draw_data.TotalVtxCount = (*dl)->VtxBuffer.size();
    draw_data.TotalIdxCount = (*dl)->IdxBuffer.size();
    draw_data.DisplayPos = pos;
    draw_data.DisplaySize = size;
    draw_data.FramebufferScale = ImVec2(1, 1);

    return draw_data;
}

// Render drawlist using the backend's renderer
void renderDrawList(GLuint fbo, ImDrawList* dl, ImVec2 pos, ImVec2 size) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glViewport(0, 0, size.x, size.y);
    glClearColor(.5f, .5f, .5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    ImDrawData dd = makeDrawData(&dl, pos, size);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplOpenGL3_RenderDrawData(&dd);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
// --------------------------------------------------------------------------------

static void SetImGuiTooltip(const ImTextureID tex_id, const ImVec2& pos, ImGuiIO& io, float tex_w, float tex_h)
{
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
    ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        float region_sz = 30.0f;
        float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
        //float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
        float region_y = tex_h - (io.MousePos.y - pos.y) - region_sz * 0.5f; // flip y
        float zoom = 3.0f;
        if (region_x < 0.0f) { region_x = 0.0f; }
        else if (region_x > tex_w - region_sz) { region_x = tex_w - region_sz; }
        if (region_y < 0.0f) { region_y = 0.0f; }
        else if (region_y > tex_h - region_sz) { region_y = tex_h - region_sz; }
        //ImVec2 uv0 = ImVec2((region_x) / tex_w, region_y / tex_h);
        //ImVec2 uv1 = ImVec2((region_x + region_sz) / tex_w, (region_y + region_sz) / tex_h);
        ImVec2 uv0 = ImVec2((region_x) / tex_w, (region_y + region_sz) / tex_h); // flip y
        ImVec2 uv1 = ImVec2((region_x + region_sz) / tex_w, (region_y) / tex_h);  // flip y
        ImGui::Image(tex_id, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tint_col, border_col);
        ImGui::GetWindowDrawList()->AddImage((ImTextureID)aim_tex, ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); // render aim icon on the image item rect position
        ImGui::EndTooltip();
    }
}

PanoLayer::PanoLayer() {}

void PanoLayer::OnAttach()
{
    FileManager::LoadTextureFromFile("assets/img/aim.png", &aim_tex, &aim_w, &aim_h,0);
}

void PanoLayer::OnUIRender()
{
    // Panorama view
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Begin("Viewport", NULL); // ImGuiWindowFlags_NoScrollbar?
    ImGuiIO& io = ImGui::GetIO();
    auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
    auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();

    auto viewportOffset = ImGui::GetWindowPos();
    m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
    m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

    auto& left_image = ToolLayer::s_FileManager.GetPano01Texture();
    auto& right_image = ToolLayer::s_FileManager.GetPano02Texture();

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y) // update when viewport resized
    {
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
        
        //m_ratio = m_ViewportSize[0] / left_image.width; // resized by width
        m_ratio = m_ViewportSize[1] / (2.0 * left_image.height); // resize by half viewportsize
        
        m_fboSize = ImVec2((float)left_image.width * m_ratio, (float)left_image.height * m_ratio * 2);
        makeFBO(m_fboSize, &fbo, &tex);
    }

    ImTextureID my_left_tex_id = (ImTextureID)left_image.texID;
    ImTextureID my_right_tex_id = (ImTextureID)right_image.texID;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float my_tex_w = (float)left_image.width * m_ratio;
    float my_tex_h = (float)left_image.height * m_ratio;

    ImGui::Image(my_left_tex_id, ImVec2{ my_tex_w, my_tex_h });
    SetImGuiTooltip((ImTextureID)tex, pos, io, m_fboSize.x, m_fboSize.y); // TODO: fix
    ImGui::Image(my_right_tex_id, ImVec2{ my_tex_w, my_tex_h });
    SetImGuiTooltip((ImTextureID)tex, pos, io, my_tex_w, my_tex_h * 2);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Draw Corner Points
    ImVec2 viewport_bound{ m_ViewportBounds[0].x, m_ViewportBounds[0].y };
    ToolLayer::drawWalls(pano_0_walls, draw_list, viewport_bound, m_ratio, false);
    ToolLayer::drawWalls(pano_1_walls, draw_list, viewport_bound, m_ratio, true);
    //ToolLayer::s_FileManager.GetPano01Corners().RenderCorners(draw_list, viewport_bound, m_ratio, false);
    //ToolLayer::s_FileManager.GetPano02Corners().RenderCorners(draw_list, viewport_bound, m_ratio, true);
    ToolLayer::s_FileManager.GetTransformCorners().RenderCorners(draw_list, viewport_bound, m_ratio, false, true); // transformed corners


    // draw matching points
    for (int i = 0; i < ToolLayer::s_MatchPoints.size(); ++i)
    {
        //auto& [left_match, right_match] = matching_points[i];
        glm::vec2& left_match = ToolLayer::s_MatchPoints.left_pixels[i];
        glm::vec2& right_match = ToolLayer::s_MatchPoints.right_pixels[i];

        const ImU32 col_rand = ToolLayer::s_MatchPoints.v_color[i];
        draw_list->AddCircleFilled(ImVec2{ m_ViewportBounds[0].x + (int)(left_match.x+po_0.first)%1024 * m_ratio, m_ViewportBounds[0].y + left_match.y * m_ratio }, 5.0f * m_ratio, col_rand);
        draw_list->AddCircleFilled(ImVec2{ m_ViewportBounds[0].x + (int)(right_match.x+po_1.first)%1024 * m_ratio, m_ViewportBounds[0].y + (right_match.y + 512) * m_ratio }, 5.0f * m_ratio, col_rand);
    }

    // draw mouse picking circles
    static float sz = 30.0f;
    static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
    const ImU32 col = ImColor(colf);
    const float thickness = 3.0f;
    if (s_left_pixel.x >= 0 && s_left_pixel.y >= 0)
    {
        draw_list->AddCircle(ImVec2{ m_ViewportBounds[0].x + s_left_pixel.x * m_ratio , m_ViewportBounds[0].y + s_left_pixel.y * m_ratio }, sz * 0.5f * m_ratio, col, 0, thickness);
        draw_list->AddCircleFilled(ImVec2{ m_ViewportBounds[0].x + s_left_pixel.x * m_ratio , m_ViewportBounds[0].y + s_left_pixel.y * m_ratio }, sz * 0.06f * m_ratio, ImColor(255, 0, 0));
    }
    if (s_right_pixel.x >= 0 && s_right_pixel.y >= 0)
    {
        draw_list->AddCircle(ImVec2{ m_ViewportBounds[0].x + s_right_pixel.x * m_ratio , m_ViewportBounds[0].y + (s_right_pixel.y + 512.0f) * m_ratio }, sz * 0.5f * m_ratio, col, 0, thickness);
        draw_list->AddCircleFilled(ImVec2{ m_ViewportBounds[0].x + s_right_pixel.x * m_ratio , m_ViewportBounds[0].y + (s_right_pixel.y + 512.0f) * m_ratio }, sz * 0.06f * m_ratio, ImColor(255, 0, 0));
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    // Render to texture
    renderDrawList(fbo, draw_list, ImVec2{ m_ViewportBounds[0].x, m_ViewportBounds[0].y }, m_fboSize);
}

void PanoLayer::OnUpdate()
{
    auto [mouseX, mouseY] = ImGui::GetMousePos();
    mouseX -= m_ViewportBounds[0].x;
    mouseY -= m_ViewportBounds[0].y;
    glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];

    // mouse picking
    if (mouseX >= 0 && mouseY >= 0 && mouseX < viewportSize.x && mouseY < 1024.0f * m_ratio)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            const float snap_threshold = 10.0f;  //distance (pixel-wise) threshold for snapping to a point
            if (mouseY < 512 * m_ratio)  //first panorama?
            {
                //snap to close-by corner points?
                CornerPoints& cps = ToolLayer::s_FileManager.GetPano01Corners();
                bool snapped = false;

                glm::vec2 p;
                p.x = glm::min(mouseX / m_ratio, 1023.0f);
                p.y = glm::min(mouseY / m_ratio, 511.0f);
                
                bool Case = 0;
                for (int Case = 0; Case < 2; Case++)  //ceil and floor
                {
                    std::vector<glm::vec2>* pixels = NULL;
                    std::vector<glm::vec3>* poss = NULL;
                    if (Case == 0)
                    {
                        pixels = &cps.ceil_pixels;
                        poss = &cps.ceil_positions;
                    }
                    else
                    {
                        pixels = &cps.floor_pixels;
                        poss = &cps.floor_positions;
                    }

                    for (int ii = 0; ii < (*pixels).size(); ii++)
                    {
                        glm::vec2 P = (*pixels)[ii];

                        float len = glm::length((p - P));
                        if (len <= snap_threshold)
                        {
                            //snap!
                            s_left_pixel = P;
                            s_left_pos = (*poss)[ii];  //also use the 3d position!
                            snapped = true;
                            char str[1000] = { NULL };
                            //sprintf(str, "Snapped %.0f,%.0f -> %.0f,%.0f (%d)", p.x, p.y, P.x, P.y, ii);
                            //AddTextToShow(str);
                            //ToolLayer::s_TextLog.AddLog(str);
                            break;
                        }
                    }
                    if (snapped)
                        break;
                }

                if (!snapped)
                {
                    s_left_pixel.x = glm::min(mouseX / m_ratio, 1023.0f);
                    s_left_pixel.y = glm::min(mouseY / m_ratio, 511.0f);
                    s_left_pos = glm::vec3(0);
                }
            }
            else  //second panorama?
            {
                //snap to close-by corner points?
                CornerPoints& cps = ToolLayer::s_FileManager.GetPano02Corners();
                bool snapped = false;

                glm::vec2 p;
                p.x = glm::min(mouseX / m_ratio, 1023.0f);
                p.y = glm::min((mouseY - 512.0f * m_ratio) / m_ratio, 511.0f);

                bool Case = 0;
                for (int Case = 0; Case < 2; Case++)  //ceil and floor
                {
                    std::vector<glm::vec2>* pixels = NULL;
                    std::vector<glm::vec3>* poss = NULL;
                    if (Case == 0)
                    {
                        pixels = &cps.ceil_pixels;
                        poss = &cps.ceil_positions;
                    }
                    else
                    {
                        pixels = &cps.floor_pixels;
                        poss = &cps.floor_positions;
                    }

                    for (int ii = 0; ii < (*pixels).size(); ii++)
                    {
                        glm::vec2 P = (*pixels)[ii];

                        float len = glm::length((p - P));
                        if (len <= snap_threshold)
                        {
                            //snap!
                            s_right_pixel = P;
                            s_right_pos = (*poss)[ii];  //also use the 3d position!
                            snapped = true;
                            char str[1000] = { NULL };
                            //sprintf(str, "Snapped %.0f,%.0f -> %.0f,%.0f (%d)", p.x, p.y, P.x, P.y, ii);
                            //AddTextToShow(str);
                            //ToolLayer::s_TextLog.AddLog(str);
                            break;
                        }
                    }
                    if (snapped)
                        break;
                }

                if (!snapped)
                {
                    s_right_pixel.x = glm::min(mouseX / m_ratio, 1023.0f);
                    s_right_pixel.y = glm::min((mouseY - 512.0f * m_ratio) / m_ratio, 511.0f);
                    s_right_pos = glm::vec3(0);
                }
            }
        }
    }
}

Application* CreateApplication(int argc, char** argv)
{
    ApplicationSpecification spec;

    Application* app = new Application(spec);
    app->PushLayer<PanoLayer>();
    app->PushLayer<ToolLayer>();
    return app;
}

glm::vec2 PanoLayer::s_left_pixel(-1.0f, -1.0f);
glm::vec2 PanoLayer::s_right_pixel(-1.0f, -1.0f);
glm::vec3 PanoLayer::s_left_pos(0);
glm::vec3 PanoLayer::s_right_pos(0);
bool PanoLayer::isMouseButtonLeftClick = false;