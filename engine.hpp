#pragma once

#include <vector>
#include <cstdint>

constexpr float NEAR_PLANE = 0.1f;

constexpr float MOUSE_SENSITIVITY = 0.003f;
constexpr float CAMERA_SPEED = 3.0f;

constexpr float CUBE_FOV = 400.0f;
constexpr float CUBE_ROTATION_SPEED = 1.2f;

struct Window {
    int width;
    int height;
    std::vector<uint32_t> pixels;
};

struct Vec3 {
    float x, y, z;
};

struct Camera {
    Vec3 position = { 0.0f, 0.0f, 0.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct Point {
    int x;
    int y;
    bool visible;
};

void rotate_x(float& y, float& z, float angle);
void rotate_y(float& x, float& z, float angle);

void put_pixel(Window& window, int x, int y, uint32_t color);
void draw_line(Window& window, int x0, int y0, int x1, int y1, uint32_t color);
void draw_cube(Window& window, const Camera& camera, float angle);

void move_camera(Camera& camera, float dt);
