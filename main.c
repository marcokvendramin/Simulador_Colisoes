#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

struct Vec3 {
    float x, y, z;
    Vec3 operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    float dot(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const {
        float len = length();
        return (len == 0) ? Vec3{0,0,0} : (*this / len);
    }
};

struct Ball {
    Vec3 pos, vel;
    float radius, mass;
    SDL_Color color;
};

int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 800;
float BOX_SIZE = 500.0f, FOCAL_LENGTH = 600.0f, CAMERA_Z = 700.0f, DT = 0.016f, CR = 1.0f;
int N_BALLS = 10;

SDL_Point project(const Vec3& p) {
    float scale = FOCAL_LENGTH / (CAMERA_Z - p.z);
    return {
        static_cast<int>(p.x * scale + SCREEN_WIDTH / 2),
        static_cast<int>(p.y * scale + SCREEN_HEIGHT / 2)
    };
}

void updatePosition(Ball& ball) {
    ball.pos = ball.pos + ball.vel * DT;
}

void handleWallCollision(Ball& b) {
    float half = BOX_SIZE / 2.0f;
    for (int i = 0; i < 3; ++i) {
        float* p = (i == 0 ? &b.pos.x : (i == 1 ? &b.pos.y : &b.pos.z));
        float* v = (i == 0 ? &b.vel.x : (i == 1 ? &b.vel.y : &b.vel.z));
        if (*p - b.radius < -half) {
            *p = -half + b.radius;
            *v = -*v * CR;
        }
        if (*p + b.radius > half) {
            *p = half - b.radius;
            *v = -*v * CR;
        }
    }
}

void handleBallCollision(Ball& a, Ball& b) {
    Vec3 delta = b.pos - a.pos;
    float dist = delta.length();
    float minDist = a.radius + b.radius;

    if (dist < minDist && dist > 0.0f) {
        Vec3 n = delta / dist;
        Vec3 v_rel = a.vel - b.vel;
        float vel_along_normal = v_rel.dot(n);

        if (vel_along_normal > 0) return;

        float m1 = a.mass, m2 = b.mass;
        float j = -(1 + CR) * vel_along_normal / (1 / m1 + 1 / m2);
        Vec3 impulse = n * j;

        a.vel = a.vel + impulse / m1;
        b.vel = b.vel - impulse / m2;

        Vec3 correction = n * (minDist - dist) * 0.5f;
        a.pos = a.pos - correction;
        b.pos = b.pos + correction;

        // Change color on collision
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        auto randColor = [&]() -> SDL_Color {
            return {
                static_cast<Uint8>(rng() % 256),
                static_cast<Uint8>(rng() % 256),
                static_cast<Uint8>(rng() % 256),
                255
            };
        };
        a.color = randColor();
        b.color = randColor();
    }
}

SDL_Texture* renderText(SDL_Renderer* ren, TTF_Font* font, const std::string& msg, SDL_Color color) {
    SDL_Surface* surf = TTF_RenderText_Blended(font, msg.c_str(), color);
    if (!surf) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    return tex;
}

int main() {
    // Parâmetros fixos (sem interação)
    N_BALLS = 10;
    SCREEN_WIDTH = 800;
    SCREEN_HEIGHT = 800;
    DT = 0.016f;
    CR = 0.9f;  // coeficiente de restituição fixo

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window* win = SDL_CreateWindow("Energia Cinética - Simulação 3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    TTF_Font* font = TTF_OpenFont("DejaVuSans.ttf", 18);
    if (!font) {
        std::cerr << "Erro ao carregar a fonte DejaVuSans.ttf\n";
        return 1;
    }

    std::vector<Ball> balls;
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> pdist(-BOX_SIZE / 2 + 50, BOX_SIZE / 2 - 50);
    std::uniform_real_distribution<float> vdist(-100, 100);
    std::uniform_real_distribution<float> rdist(10, 30);

    for (int i = 0; i < N_BALLS; ++i) {
        Ball b;
        b.radius = rdist(rng);
        b.mass = b.radius * b.radius * b.radius;
        int attempts = 0;
        const int max_attempts = 1000;
        bool valid = false;
        while (!valid && attempts < max_attempts) {
            b.pos = {pdist(rng), pdist(rng), pdist(rng)};
            b.vel = {vdist(rng), vdist(rng), vdist(rng)};
            valid = true;
            for (const Ball& other : balls) {
                if ((b.pos - other.pos).length() < b.radius + other.radius) {
                    valid = false;
                    break;
                }
            }
            attempts++;
        }
        if (!valid) {
            std::cout << "Aviso: nao foi possivel posicionar bola " << i+1
                      << " apos " << max_attempts << " tentativas.\n";
        }
        b.color = {
            static_cast<Uint8>(rng() % 256),
            static_cast<Uint8>(rng() % 256),
            static_cast<Uint8>(rng() % 256),
            255
        };
        balls.push_back(b);
    }

    SDL_Event e;
    bool running = true;
    SDL_Color white = {255, 255, 255, 255};

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        for (auto& b : balls) {
            updatePosition(b);
            handleWallCollision(b);
        }

        for (size_t i = 0; i < balls.size(); ++i)
            for (size_t j = i + 1; j < balls.size(); ++j)
                handleBallCollision(balls[i], balls[j]);

        float energiaCineticaTotal = 0;
        for (auto& b : balls) {
            energiaCineticaTotal += 0.5f * b.mass * (b.vel.x * b.vel.x + b.vel.y * b.vel.y + b.vel.z * b.vel.z);
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        for (auto& b : balls) {
            SDL_Point p = project(b.pos);
            float scale = FOCAL_LENGTH / (CAMERA_Z - b.pos.z);
            int r = static_cast<int>(b.radius * scale);
            if (r > 0) {
                SDL_SetRenderDrawColor(ren, b.color.r, b.color.g, b.color.b, 255);
                for (int dx = -r; dx <= r; ++dx)
                    for (int dy = -r; dy <= r; ++dy)
                        if (dx*dx + dy*dy <= r*r)
                            SDL_RenderDrawPoint(ren, p.x + dx, p.y + dy);
            }
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2)
           << "Energia Cinetica Total = " << energiaCineticaTotal;
        SDL_Texture* tex = renderText(ren, font, ss.str(), white);

        if (tex) {
            int w, h;
            SDL_QueryTexture(tex, NULL, NULL, &w, &h);
            SDL_Rect rect = {10, 10, w, h};
            SDL_RenderCopy(ren, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
