#include "projectile.h"
#include <math.h>

void CreateProjectile(Game *g, Vector2 origin, int targetIdx, float damage, float splash,
                      TowerType src) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (g->projectiles[i].active)
            continue;
        Projectile *p = &g->projectiles[i];
        p->position = origin;
        p->damage = damage;
        p->splashRadius = splash;
        p->targetIndex = targetIdx;
        p->sourceType = src;
        p->active = true;
        switch (src) {
        case TOWER_BASIC:
            p->speed = 450.0f;
            p->color = SKYBLUE;
            break;
        case TOWER_SNIPER:
            p->speed = 750.0f;
            p->color = LIME;
            break;
        case TOWER_SPLASH:
            p->speed = 300.0f;
            p->color = ORANGE;
            break;
        default:
            p->speed = 450.0f;
            p->color = WHITE;
            break;
        }
        return;
    }
}

/* ============================================================
 * T23 — SpawnParticles
 * ============================================================ */

/* Belirtilen pozisyonda rastgele yönlerde parçacıklar oluşturur. */

        void DrawProjectiles(Game * g) {
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                Projectile *p = &g->projectiles[i];
                if (!p->active)
                    continue;
                float radius = (p->sourceType == TOWER_SNIPER) ? 3.0f : 5.0f;

                /* T58 — Kuyruk efekti: eski pozisyonları azalan opaklık/boyutla çiz */
                for (int t = 0; t < TRAIL_LENGTH; t++) {
                    int idx = (p->trailIndex - t - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
                    float age = (float)(t + 1) / (float)TRAIL_LENGTH; /* 0 = taze, 1 = eski */
                    float tr = radius * (1.0f - age * 0.75f);
                    if (tr < 0.5f)
                        continue;
                    Color tc = p->color;
                    tc.a = (unsigned char)(160.0f * (1.0f - age));
                    DrawCircleV(p->trailPositions[idx], tr, tc);
                }

                DrawCircleV(p->position, radius, p->color);
            }
        }

        /* ============================================================
         * T25 — DrawParticles
         * ============================================================ */
