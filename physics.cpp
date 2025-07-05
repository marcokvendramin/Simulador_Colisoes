#include "physics.h"
#include <math.h>
#include <stdlib.h>

float calcula_distancia(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

int verifica_sobreposicao(Bola3D *a, Bola3D *b) {
    float dist = calcula_distancia(a->x, a->y, a->z, b->x, b->y, b->z);
    return dist < (a->raio + b->raio);
}

int bolas_sobrepostas(Bola3D *bolas, int n) {
    for(int i = 0; i < n; i++) {
        for(int j = i+1; j < n; j++) {
            if(verifica_sobreposicao(&bolas[i], &bolas[j])) return 1;
        }
    }
    return 0;
}

void init_bolas(Bola3D *bolas, int n, int largura, int altura, int profundidade) {
    int i;
    for(i = 0; i < n; i++) {
        do {
            bolas[i].raio = (rand() % 41) + 10; // raio 10 a 50
            bolas[i].x = (float)(rand() % (largura - 2*(int)bolas[i].raio) + (int)bolas[i].raio);
            bolas[i].y = (float)(rand() % (altura - 2*(int)bolas[i].raio) + (int)bolas[i].raio);
            bolas[i].z = (float)(rand() % (profundidade - 2*(int)bolas[i].raio) + (int)bolas[i].raio);
            bolas[i].vx = (float)(rand() % 21 - 10);
            bolas[i].vy = (float)(rand() % 21 - 10);
            bolas[i].vz = (float)(rand() % 21 - 10);
            bolas[i].massa = bolas[i].raio * 0.5f;
            bolas[i].cor.r = rand() % 256;
            bolas[i].cor.g = 0;
            bolas[i].cor.b = rand() % 256;
            bolas[i].cor.a = 255;
        } while (i > 0 && bolas_sobrepostas(bolas, i+1));
    }
}

void atualiza_posicao(Bola3D *bola, float dt) {
    bola->x += bola->vx * dt;
    bola->y += bola->vy * dt;
    bola->z += bola->vz * dt;
}

void trata_colisao_parede(Bola3D *bola, int largura, int altura, int profundidade) {
    if(bola->x >= largura - bola->raio) {
        bola->x = largura - bola->raio;
        bola->vx *= -1;
    } else if(bola->x <= bola->raio) {
        bola->x = bola->raio;
        bola->vx *= -1;
    }

    if(bola->y >= altura - bola->raio) {
        bola->y = altura - bola->raio;
        bola->vy *= -1;
    } else if(bola->y <= bola->raio) {
        bola->y = bola->raio;
        bola->vy *= -1;
    }

    if(bola->z >= profundidade - bola->raio) {
        bola->z = profundidade - bola->raio;
        bola->vz *= -1;
    } else if(bola->z <= bola->raio) {
        bola->z = bola->raio;
        bola->vz *= -1;
    }
}

void trata_colisao_bolas(Bola3D *a, Bola3D *b, float CR) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dz = b->z - a->z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    float min_dist = a->raio + b->raio;

    if(dist < min_dist && dist > 0.0f) {
        // Normaliza vetor diretor
        float nx = dx / dist;
        float ny = dy / dist;
        float nz = dz / dist;

        // Projeta velocidades na direção normal
        float va_proj = a->vx * nx + a->vy * ny + a->vz * nz;
        float vb_proj = b->vx * nx + b->vy * ny + b->vz * nz;

        // Velocidade do centro de massa na direção da colisão
        float vcm = (a->massa * va_proj + b->massa * vb_proj) / (a->massa + b->massa);

        // Novas velocidades projetadas com coeficiente de restituição
        float va_proj_novo = (1 + CR) * vcm - CR * va_proj;
        float vb_proj_novo = (1 + CR) * vcm - CR * vb_proj;

        // Componentes tangenciais (perpendiculares ao vetor diretor)
        // Um vetor tangente qualquer: t1 = (-ny, nx, 0) (se vetor diretor for paralelo ao z, pode trocar)
        // Mas melhor usar a fórmula para vetor perpendicular em 3D:
        // Componentes tangenciais são a parte da velocidade que NÃO está na direção (nx, ny, nz)
        // Então vamos calcular vetor tangente subtraindo a projeção normal

        // Para a
        float vax = a->vx - va_proj * nx;
        float vay = a->vy - va_proj * ny;
        float vaz = a->vz - va_proj * nz;

        // Para b
        float vbx = b->vx - vb_proj * nx;
        float vby = b->vy - vb_proj * ny;
        float vbz = b->vz - vb_proj * nz;

        // Atualiza velocidades somando tangencial + nova normal
        a->vx = vax + va_proj_novo * nx;
        a->vy = vay + va_proj_novo * ny;
        a->vz = vaz + va_proj_novo * nz;

        b->vx = vbx + vb_proj_novo * nx;
        b->vy = vby + vb_proj_novo * ny;
        b->vz = vbz + vb_proj_novo * nz;

        // Ajusta posição para evitar sobreposição
        float overlap = 0.5f * (min_dist - dist);
        a->x -= overlap * nx;
        a->y -= overlap * ny;
        a->z -= overlap * nz;

        b->x += overlap * nx;
        b->y += overlap * ny;
        b->z += overlap * nz;
    }
}

void desenha_circulo(SDL_Renderer *renderer, int cx, int cy, int raio, SDL_Color cor) {
    SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);

    int x = raio - 1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (raio << 1);

    while(x >= y) {
        SDL_RenderDrawPoint(renderer, cx + x, cy + y);
        SDL_RenderDrawPoint(renderer, cx + y, cy + x);
        SDL_RenderDrawPoint(renderer, cx - y, cy + x);
        SDL_RenderDrawPoint(renderer, cx - x, cy + y);
        SDL_RenderDrawPoint(renderer, cx - x, cy - y);
        SDL_RenderDrawPoint(renderer, cx - y, cy - x);
        SDL_RenderDrawPoint(renderer, cx + y, cy - x);
        SDL_RenderDrawPoint(renderer, cx + x, cy - y);

        if(err <= 0) {
            y++;
            err += dy;
            dy += 2;
        }
        if(err > 0) {
            x--;
            dx += 2;
            err += dx - (raio << 1);
        }
    }
}

float energia_cinetica(Bola3D *bola) {
    return 0.5f * bola->massa * (bola->vx * bola->vx + bola->vy * bola->vy + bola->vz * bola->vz);
}
