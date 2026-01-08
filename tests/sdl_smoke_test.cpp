#include <SDL.h>
#include <SDL_ttf.h>
#include <frameflow/layout.hpp>
#include <iostream>
#include <string>

using namespace frameflow;

static SDL_FRect to_sdl(const Rect &r) {
    return SDL_FRect{r.origin.x, r.origin.y, r.size.x, r.size.y};
}

// Map NodeType to color (R,G,B)
static SDL_Color get_type_color(NodeType type) {
    switch (type) {
        case NodeType::Generic: return {255, 0, 0, 255};    // red
        case NodeType::Center:  return {0, 255, 0, 255};    // green
        case NodeType::Box:     return {0, 0, 255, 255};    // blue
        case NodeType::Flow:    return {255, 255, 0, 255};  // yellow
        default:                return {255, 255, 255, 255};
    }
}

// Draw borders recursively with color
static void DrawBorders(SDL_Renderer *renderer, TTF_Font *font, System &canvas, NodeId node_id) {
    Node &node = get_node(canvas, node_id);

    SDL_Color color = get_type_color(node.type);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    SDL_FRect rect = to_sdl(node.bounds);
    SDL_RenderDrawRectF(renderer, &rect);

    // Draw label text in top-left corner
    if (font) {
        std::string label;
        switch (node.type) {
            case NodeType::Generic: label = "Generic"; break;
            case NodeType::Center:  label = "Center"; break;
            case NodeType::Box:     label = "Box"; break;
            case NodeType::Flow:    label = "Flow"; break;
            default: label = "?";
        }

        SDL_Surface* surface = TTF_RenderText_Blended(font, label.c_str(), color);
        if (surface) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect dst{ int(node.bounds.origin.x + 2), int(node.bounds.origin.y + 2), surface->w, surface->h };
            SDL_RenderCopy(renderer, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surface);
        }
    }

    for (auto child_id : node.children) {
        DrawBorders(renderer, font, canvas, child_id);
    }
}

// Draw grid stays the same
static void DrawGrid(SDL_Renderer *renderer, int w, int h, int spacing) {
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    for (int x = 0; x < w; x += spacing)
        SDL_RenderDrawLine(renderer, x, 0, x, h);
    for (int y = 0; y < h; y += spacing)
        SDL_RenderDrawLine(renderer, 0, y, w, y);
}

int main(int, char **) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    auto viewport_size = float2{1920, 1280};

    SDL_Window *window = SDL_CreateWindow(
        "frameflow layout test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        static_cast<int>(viewport_size.x),
        static_cast<int>(viewport_size.y),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    // Load font for labels
    TTF_Font* font = TTF_OpenFont("arial.ttf", 16);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << "\n";
    }

    System canvas;

    const NodeId root_id = add_generic(canvas, NullNode);
    Node &root_node = get_node(canvas, root_id);
    get_node(canvas, root_id).minimum_size = viewport_size;
    get_node(canvas, root_id).bounds.size = viewport_size;

    // Center child
    const NodeId center_id = add_center(canvas, root_id);
    get_node(canvas, center_id).minimum_size = {500*2, 400*2};

    // Horizontal box
    const NodeId hbox_id = add_box(canvas, center_id, BoxData{Direction::Horizontal, Align::SpaceBetween});
    get_node(canvas, hbox_id).minimum_size = {500, 80*2};

    auto populate = [&](const NodeId parent, const int count, const int min, const int max) {
        for (int i = 0; i < count; ++i) {
            NodeId c = add_generic(canvas, parent);
            auto s = 100.f;
            get_node(canvas, c).minimum_size = {s, s};
        }
    };
    populate(hbox_id,   3, 20, 60);

    int window_width = static_cast<int>(viewport_size.x);
    int window_height = static_cast<int>(viewport_size.y);
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                window_width = event.window.data1;
                window_height = event.window.data2;
                std::cout<<window_width << "x"<< window_height <<std::endl;

                // Update root node to new window size
                Node &root = get_node(canvas, root_id);
                root.bounds.size = { float(window_width), float(window_height) };
                //root.minimum_size = { float(window_width), float(window_height) };
                viewport_size.x = (float)window_width;
                viewport_size.y = (float)window_height;
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        Uint64 t0 = SDL_GetPerformanceCounter();
        compute_layout(canvas, root_id);
        Uint64 t1 = SDL_GetPerformanceCounter();
        double ms = (double)(t1 - t0) * 1000.0 / (double)SDL_GetPerformanceFrequency();

        static double accum_ms = 0.0;
        static int sample_count = 0;
        accum_ms += ms;
        sample_count++;
        if (sample_count == 60) {
            printf("compute_layout: %.4f ms (avg over 60 frames)\n", accum_ms/60.0);
            accum_ms = 0.0;
            sample_count = 0;
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        DrawGrid(renderer, viewport_size.x, viewport_size.y, 50);
        DrawBorders(renderer, font, canvas, root_id);

        SDL_RenderPresent(renderer);
    }

    if (font) TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
