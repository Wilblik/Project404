#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 1024;
constexpr int CENTER_X = WINDOW_WIDTH / 2;
constexpr int CENTER_Y = WINDOW_HEIGHT / 2;

constexpr float MOUSE_SENSITIVITY = 0.003f;
constexpr float CAMERA_SPEED = 0.05f;

constexpr float NEAR_PLANE = 0.1f;
constexpr float CUBE_FOV = 400.0f;

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

void put_pixel(std::vector<uint32_t>& pixels, int x, int y, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT) return;
    int idx = (y * WINDOW_WIDTH) + x;
    pixels[idx] = color;
}

void draw_line(std::vector<uint32_t>& pixels, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = std::abs(dx) > std::abs(dy) ? std::abs(dx) : std::abs(dy);

    if (steps == 0) {
        put_pixel(pixels, x0, y0, color);
    }

    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;

    float x = x0;
    float y = y0;

    for (int i = 0; i <= steps; ++i) {
        put_pixel(pixels, std::round(x), std::round(y), color);
        x += xinc;
        y += yinc;
    }
}

void draw_cube(const Camera& camera, std::vector<uint32_t> &pixels, float angle) {
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

        float x_proj = (x_view / z_view) * CUBE_FOV + CENTER_X;
        float y_proj = (y_view / z_view) * CUBE_FOV + CENTER_Y;

        projected_points.push_back({(int)std::round(x_proj), (int)std::round(y_proj), true});
    }

    for (const auto& conn : cube_connections) {
        auto start = projected_points[conn.first];
        auto end   = projected_points[conn.second];
        if (!start.visible || !end.visible) continue;
        draw_line(pixels, start.x, start.y, end.x, end.y, 0xFF00FF00);
    }
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "[ERROR] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Hello 3D Graphics!", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "[ERROR] SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        fprintf(stderr, "[ERROR] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_SetRenderVSync(renderer, 1)) {
        fprintf(stderr, "[ERROR] SDL_SetRenderVSync failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!texture) {
        fprintf(stderr, "[ERROR] SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_SetWindowRelativeMouseMode(window, true)) {
        fprintf(stderr, "[ERROR] SDL_SetWindowRelativeMouseMode failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Camera camera;
    float cube_angle = 0.0f;
    bool running = true;
    SDL_Event event;
    std::vector<uint32_t> pixels(WINDOW_WIDTH * WINDOW_HEIGHT, 0);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_Q) {
                    running = false;
                }
            }
        }

        float xrel, yrel;
        SDL_GetRelativeMouseState(&xrel, &yrel);

        camera.yaw   -= xrel * MOUSE_SENSITIVITY;
        camera.pitch -= yrel * MOUSE_SENSITIVITY;

        if (camera.pitch > 1.5f) camera.pitch = 1.5f;
        if (camera.pitch < -1.5f) camera.pitch = -1.5f;

        float forward_x = -std::sin(camera.yaw) * std::cos(camera.pitch) * CAMERA_SPEED;
        float forward_y = -std::sin(camera.pitch) * CAMERA_SPEED;
        float forward_z =  std::cos(camera.yaw) * std::cos(camera.pitch) * CAMERA_SPEED;

        float right_x   =  std::cos(camera.yaw) * CAMERA_SPEED;
        float right_z   =  std::sin(camera.yaw) * CAMERA_SPEED;

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

        if (keys[SDL_SCANCODE_A]){
            camera.position.x -= right_x;
            camera.position.z -= right_z;
        } 

        if (keys[SDL_SCANCODE_D]) {
            camera.position.x += right_x;
            camera.position.z += right_z;
        }

        if (keys[SDL_SCANCODE_SPACE])  camera.position.y -= CAMERA_SPEED;
        if (keys[SDL_SCANCODE_LSHIFT]) camera.position.y += CAMERA_SPEED;

        std::fill(pixels.begin(), pixels.end(), 0xFF000000); // Clear to Black

        draw_cube(camera, pixels, cube_angle);
        cube_angle += 0.02f;

        SDL_UpdateTexture(texture, nullptr, pixels.data(), WINDOW_WIDTH * sizeof(uint32_t));
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
