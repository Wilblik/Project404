#include "engine.hpp"

#include <SDL3/SDL.h>
#include <cstdint>
#include <cmath>

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

void draw_cube(Window& window, const Camera& camera, float angle) {
    std::vector<Vec3> cube_points = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, // Front Face (0-3)
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}   // Back Face  (4-7)
    };

    std::vector<std::pair<int, int>> cube_connections = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front Face square
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back Face square
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting the two faces
    };

    std::vector<Point> projected_points;

    for (const auto& p : cube_points) {
        float cube_x = p.x;
        float cube_y = p.y;
        float cube_z = p.z;
        rotate_y(cube_x, cube_z, angle);
        cube_z += 5.0f;

        float x_view = cube_x - camera.position.x;
        float y_view = cube_y - camera.position.y;
        float z_view = cube_z - camera.position.z;
        rotate_y(x_view, z_view, -camera.yaw);
        rotate_x(y_view, z_view, -camera.pitch);

        if (z_view <= NEAR_PLANE){
            projected_points.push_back({0, 0, false});
            continue;
        };

        float x_proj = (x_view / z_view) * CUBE_FOV + window.width / 2 ;
        float y_proj = (y_view / z_view) * CUBE_FOV + window.height / 2;

        projected_points.push_back({(int)std::round(x_proj), (int)std::round(y_proj), true});
    }

    for (const auto& conn : cube_connections) {
        auto start = projected_points[conn.first];
        auto end   = projected_points[conn.second];
        if (!start.visible || !end.visible) continue;
        draw_line(window, start.x, start.y, end.x, end.y, 0xFF00FF00);
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
