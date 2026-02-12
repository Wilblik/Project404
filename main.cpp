#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstdio>

#include "engine.hpp"

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 1024;

static SDL_Window* sdl_window;
static SDL_Renderer* sdl_renderer;
static SDL_Texture* sdl_texture;

static int init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "[ERROR] SDL_Init failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    sdl_window = SDL_CreateWindow("Hello 3D Graphics!", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!sdl_window) {
        fprintf(stderr, "[ERROR] SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    sdl_renderer = SDL_CreateRenderer(sdl_window, nullptr);
    if (!sdl_renderer) {
        fprintf(stderr, "[ERROR] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (!SDL_SetRenderVSync(sdl_renderer, 1)) {
        fprintf(stderr, "[ERROR] SDL_SetRenderVSync failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!sdl_texture) {
        fprintf(stderr, "[ERROR] SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(sdl_renderer);
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (!SDL_SetWindowRelativeMouseMode(sdl_window, true)) {
        fprintf(stderr, "[ERROR] SDL_SetWindowRelativeMouseMode failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(sdl_renderer);
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main() {
    int result = init_sdl();
    if (result < 0) return result;

    Window window = {
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .pixels = std::vector<uint32_t>(WINDOW_WIDTH * WINDOW_HEIGHT, 0)
    };

    Camera camera;
    float cube_angle = 0.0f;

    SDL_Event event;
    Uint64 last_time = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();

    bool running = true;
    while (running) {

        /* Calculate Delta Time */
        Uint64 current_time = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(current_time - last_time) / frequency;
        last_time = current_time;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_Q) {
                    running = false;
                }
            }
        }

        move_camera(camera, dt);
        std::fill(window.pixels.begin(), window.pixels.end(), 0xFF000000);
        draw_wireframe_cube(window, camera, cube_angle, 0xFF0000FF);
        draw_filled_cube(window, camera, cube_angle, 0xFFFF0000);
        cube_angle += CUBE_ROTATION_SPEED * dt;

        if (!SDL_UpdateTexture(sdl_texture, nullptr, window.pixels.data(), window.width * sizeof(uint32_t))) {
            fprintf(stderr, "[ERROR] SDL_UpdateTexture failed: %s\n", SDL_GetError());
        };

        if (!SDL_RenderTexture(sdl_renderer, sdl_texture, nullptr, nullptr)) {
            fprintf(stderr, "[ERROR] SDL_RenderTexture failed: %s\n", SDL_GetError());
        }

        if (!SDL_RenderPresent(sdl_renderer)) {
            fprintf(stderr, "[ERROR] SDL_RenderPresent failed: %s\n", SDL_GetError());
        };
    }

    SDL_DestroyTexture(sdl_texture);
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
