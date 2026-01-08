#include <SDL.h>
#include <frameflow/layout.hpp>

using namespace frameflow;

static SDL_FRect to_sdl(const frameflow::Rect& r) {
    return SDL_FRect{ r.origin.x, r.origin.y, r.size.width, r.size.height };
}

static void DrawBorders(SDL_Renderer *renderer, System&canvas, NodeId node_id) {
    Node &node = get_node(canvas, node_id);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect border_rect = to_sdl(node.bounds);
    SDL_RenderDrawRectF(renderer, &border_rect);
    for (auto child_id : node.children) {
        DrawBorders(renderer, canvas, child_id);
    }
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "frameflow SDL Bordered Rectangle",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    System canvas;
    CenterData center_data;
    NodeId root_id = add_center(canvas, NullNode, center_data);
    frameflow::Rect rect{
        0.0f,
        0.0f,
        640.0f,
        480.0f
    };
    get_node(canvas, root_id).bounds = rect;

    NodeId child_node_id = add_generic(canvas, root_id);
    auto &child_node = get_node(canvas, child_node_id);
    child_node.bounds = {{0, 0}, {100, 300}};

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Render!
        compute_layout(canvas, root_id);

        // Clear background
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        // Draw border
        DrawBorders(renderer, canvas, root_id);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
