#include "engine.hpp"

#include <SDL3/SDL.h>
#include <cmath>
#include <array>

void rotate_x(float& y, float& z, float angle) {
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);

    float ny = y * cos_a - z * sin_a;
    float nz = y * sin_a + z * cos_a;

    y = ny;
    z = nz;
}

void rotate_y(float& x, float& z, float angle) {
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);

    float nx = x * cos_a - z * sin_a;
    float nz = x * sin_a + z * cos_a;

    x = nx;
    z = nz;
}

void put_pixel(Window& window, int x, int y, uint32_t color) {
    if (x < 0 || x >= window.width || y < 0 || y >= window.height) return;
    int idx = (y * window.width) + x;
    window.pixels[idx] = color;
}

void draw_line(Window& window, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = std::abs(dx) > std::abs(dy) ? std::abs(dx) : std::abs(dy);

    if (steps == 0) {
        put_pixel(window, x0, y0, color);
    }

    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;

    float x = x0;
    float y = y0;

    for (int i = 0; i <= steps; ++i) {
        put_pixel(window, std::round(x), std::round(y), color);
        x += xinc;
        y += yinc;
    }
}

void draw_filled_triangle(Window &window, int x0, int y0, int x1, int y1,
                          int x2, int y2, uint32_t color)
{
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }

    int total_height = y2 - y0;
    if (total_height == 0) return; /* Broken triangle */

    for (int i = 0; i < total_height; ++i) {
        bool second_half = i > y1 - y0 || y1 == y0;
        int segment_height = second_half ? y2 - y1 : y1 - y0;

        float alpha = (float)i / total_height;
        float beta  = (float)(i - (second_half ? y1 - y0 : 0)) / segment_height;

        int A_x = x0 + (x2 - x0) * alpha;
        int B_x = second_half ? x1 + (x2 - x1) * beta : x0 + (x1 - x0) * beta;

        if (A_x > B_x) std::swap(A_x, B_x);

        for (int j = A_x; j <= B_x; j++) {
            put_pixel(window, j, y0 + i, color);
        }
    }
}


void draw_wireframe_cube(Window& window, const Camera& camera, float angle, uint32_t color) {
    static std::array<Vec3, 8> cube_points = {{
        {-2, -2, -2}, {2, -2, -2}, {2, 2, -2}, {-2, 2, -2}, // Front Face (0-3)
        {-2, -2, 2},  {2, -2, 2},  {2, 2, 2},  {-2, 2, 2}   // Back Face  (4-7)
    }};

    static std::array<std::pair<int, int>, 12> cube_connections = {{
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front Face square
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back Face square
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting the two faces
    }};

    std::vector<Point> projected_points;

    for (const auto& p : cube_points) {
        float x = p.x;
        float y = p.y;
        float z = p.z;

        rotate_y(x, z, angle);
        z += 5.0f;

        x -= camera.position.x;
        y -= camera.position.y;
        z -= camera.position.z;

        rotate_y(x, z, -camera.yaw);
        rotate_x(y, z, -camera.pitch);

        if (z < NEAR_PLANE) {
            projected_points.push_back({0, 0, false});
            continue;
        };

        float x_proj = (x / z) * CUBE_FOV + (float)window.width  / 2;
        float y_proj = (y / z) * CUBE_FOV + (float)window.height / 2;
        projected_points.push_back({(int)std::round(x_proj), (int)std::round(y_proj), true});
    }

    for (const auto& conn : cube_connections) {
        auto start = projected_points[conn.first];
        auto end = projected_points[conn.second];
        if (!start.visible || !end.visible) continue;
        draw_line(window, start.x, start.y, end.x, end.y, color);
    }
}

void draw_filled_cube(Window& window, const Camera& camera, float angle, uint32_t color) {
    static std::array<Vec3, 8> cube_points = {{
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, // Front Face (0-3)
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}   // Back Face  (4-7)
    }};

    static std::array<Tri, 12> tris = {{
        {0, 1, 2}, {0, 2, 3}, // Front
        {5, 4, 7}, {5, 7, 6}, // Back
        {1, 5, 6}, {1, 6, 2}, // Right
        {4, 0, 3}, {4, 3, 7}, // Left
        {3, 2, 6}, {3, 6, 7}, // Top
        {4, 5, 1}, {4, 1, 0}  // Bottom
    }};

    std::array<Vec3, 8> view_points;

    /* Transform vertices to view space */
    for (size_t i = 0; i < cube_points.size(); ++i) {
        float x = cube_points[i].x;
        float y = cube_points[i].y;
        float z = cube_points[i].z;

        rotate_y(x, z, angle);
        z += 5.0f;

        x -= camera.position.x;
        y -= camera.position.y;
        z -= camera.position.z;

        rotate_y(x, z, -camera.yaw);
        rotate_x(y, z, -camera.pitch);

        view_points[i] = {x, y, z};
    }

    /* Process triangles */
    for (const auto& tri : tris) {
        Vec3 p0 = view_points[tri.v0];
        Vec3 p1 = view_points[tri.v1];
        Vec3 p2 = view_points[tri.v2];

        /* Back face culling */
        float ax = p1.x - p0.x;
        float ay = p1.y - p0.y;
        float az = p1.z - p0.z;

        float bx = p2.x - p0.x;
        float by = p2.y - p0.y;
        float bz = p2.z - p0.z;

        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;

        /* Check if face is looking away from the camera */
        if (p0.x * nx + p0.y * ny + p0.z * nz >= 0.0f) continue;

        /* Near plane clipping check */
        if (p0.z < NEAR_PLANE || p1.z < NEAR_PLANE || p2.z < NEAR_PLANE) continue;

        auto project_to_2d = [&](Vec3 v) -> Point {
            float x_proj = (v.x / v.z) * CUBE_FOV + (float)window.width  / 2;
            float y_proj = (v.y / v.z) * CUBE_FOV + (float)window.height / 2;
            return {(int)std::round(x_proj), (int)std::round(y_proj), true};
        };

        Point p0_proj = project_to_2d(p0);
        Point p1_proj = project_to_2d(p1);
        Point p2_proj = project_to_2d(p2);

        draw_filled_triangle(window, p0_proj.x, p0_proj.y, p1_proj.x,
                             p1_proj.y, p2_proj.x, p2_proj.y, color);
    }
}

void move_camera(Camera& camera, float dt) {
    float xrel, yrel;
    SDL_GetRelativeMouseState(&xrel, &yrel);

    camera.yaw   -= xrel * MOUSE_SENSITIVITY;
    camera.pitch -= yrel * MOUSE_SENSITIVITY;

    if (camera.pitch > 1.5f) camera.pitch = 1.5f;
    if (camera.pitch < -1.5f) camera.pitch = -1.5f;

    float move_step = CAMERA_SPEED * dt;

    float forward_x = -std::sin(camera.yaw) * std::cos(camera.pitch) * move_step;
    float forward_y = -std::sin(camera.pitch) * move_step;
    float forward_z =  std::cos(camera.yaw) * std::cos(camera.pitch) * move_step;
    float right_x   =  std::cos(camera.yaw) * move_step;
    float right_z   =  std::sin(camera.yaw) * move_step;

    const bool* keys = SDL_GetKeyboardState(nullptr);

    if (keys[SDL_SCANCODE_W]) {
        camera.position.x += forward_x;
        camera.position.y += forward_y;
        camera.position.z += forward_z;
    }
    if (keys[SDL_SCANCODE_S]) {
        camera.position.x -= forward_x;
        camera.position.y -= forward_y;
        camera.position.z -= forward_z;
    }
    if (keys[SDL_SCANCODE_A]) {
        camera.position.x -= right_x;
        camera.position.z -= right_z;
    }
    if (keys[SDL_SCANCODE_D]) {
        camera.position.x += right_x;
        camera.position.z += right_z;
    }
    if (keys[SDL_SCANCODE_SPACE])  camera.position.y -= move_step;
    if (keys[SDL_SCANCODE_LSHIFT]) camera.position.y += move_step;
}
