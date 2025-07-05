#ifndef PHYSICS_H
#define PHYSICS_H

#include <SDL2/SDL.h>

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float raio;
    float massa;
    SDL_Color cor;
} Bola3D;

void init_bolas(Bola3D *bolas, int n, int largura, int altura, int profundidade);
int verifica_sobreposicao(Bola3D *a, Bola3D *b);
int bolas_sobrepostas(Bola3D *bolas, int n);
void atualiza_posicao(Bola3D *bola, float dt);
void trata_colisao_parede(Bola3D *bola, int largura, int altura, int profundidade);
void trata_colisao_bolas(Bola3D *a, Bola3D *b, float CR);
float calcula_distancia(float x1, float y1, float z1, float x2, float y2, float z2);
void desenha_circulo(SDL_Renderer *renderer, int cx, int cy, int raio, SDL_Color cor);
float energia_cinetica(Bola3D *bola);

#endif
