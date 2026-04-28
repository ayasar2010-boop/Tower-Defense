#include "particle.h"
#include "audio.h"
#include "hud.h"
#include "util.h"
#include <math.h>

void SpawnParticles(Game *g, Vector2 pos, Color color, int count, float speed, float lifetime) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (g->particles[i].active)
            continue;
        Particle *p = &g->particles[i];
        float angle = (float)(GetRandomValue(0, 359)) * DEG2RAD;
        float spd = speed * (0.5f + (float)GetRandomValue(0, 100) / 100.0f);
        p->position = pos;
        p->velocity = (Vector2){cosf(angle) * spd, sinf(angle) * spd};
        p->lifetime = lifetime;
        p->maxLifetime = lifetime;
        p->color = color;
        p->size = 4.0f + (float)GetRandomValue(0, 4);
        p->active = true;
        spawned++;
    }
}

/* ============================================================
 * T16 — FindNearestEnemy
 * ============================================================ */

/* Verilen pozisyon ve menzil içindeki en yakın aktif düşmanın indeksini döner.
 * Hiçbiri yoksa -1 döner. */

void UpdateProjectiles(Game *g, float dt) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &g->projectiles[i];
        if (!p->active)
            continue;

        Enemy *target = NULL;
        if (!p->isEnemyProjectile && p->targetIndex >= 0 && p->targetIndex < MAX_ENEMIES)
            target = &g->enemies[p->targetIndex];

        /* T58 — kuyruk: mevcut pozisyonu kaydet */
        p->trailPositions[p->trailIndex] = p->position;
        p->trailIndex = (p->trailIndex + 1) % TRAIL_LENGTH;

        if (target && target->active) {
            /* Hedefe yönlen */
            p->velocity =
                Vec2Scale(Vec2Normalize(Vec2Subtract(target->position, p->position)), p->speed);
        }
        p->position = Vec2Add(p->position, Vec2Scale(p->velocity, dt));

        /* Dünya sınırı dışına çıktıysa deaktive et (izometrik world coords) */
        if (p->position.x < -200 || p->position.x > 2600 || p->position.y < -200 ||
            p->position.y > 1400) {
            p->active = false;
            continue;
        }

        /* Düşman Mermisi Kontrolü (Boss Phase 1 vb.) */
        if (p->isEnemyProjectile) {
            bool hit = false;
            /* Kulelere isabet kontrolü */
            for (int t = 0; t < MAX_TOWERS; t++) {
                if (!g->towers[t].active)
                    continue;
                if (Vec2Distance(p->position, g->towers[t].position) < 20.0f) {
                    g->towers[t].fireCooldown += 2.0f; /* 2 sn stun */
                    hit = true;
                }
            }
            /* Dost birimlere isabet kontrolü */
            for (int f = 0; f < MAX_FRIENDLY_UNITS; f++) {
                if (!g->friendlyUnits[f].active)
                    continue;
                if (Vec2Distance(p->position, g->friendlyUnits[f].position) < 15.0f) {
                    g->friendlyUnits[f].hp -= p->damage;
                    hit = true;
                }
            }
            if (g->hero.alive && Vec2Distance(p->position, g->hero.position) < 15.0f) {
                g->hero.stats.hp -= p->damage;
                hit = true;
            }
            if (hit) {
                SpawnParticles(g, p->position, p->color, 5, 80.0f, 0.2f);
                p->active = false;
            }
            continue;
        }

        /* İsabet kontrolü */
        if (target && target->active &&
            Vec2Distance(p->position, target->position) < target->radius + 4.0f) {

            if (p->splashRadius > 0.0f) {
                /* Splash hasar: mesafeye göre azalan hasar */
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (!g->enemies[j].active)
                        continue;
                    float dist = Vec2Distance(p->position, g->enemies[j].position);
                    if (dist < p->splashRadius) {
                        float dmg = p->damage * (1.0f - dist / p->splashRadius);
                        g->enemies[j].currentHp -= dmg;
                        SpawnFloatingText(g, g->enemies[j].position, dmg, false, false); /* T57 */
                    }
                }
                SpawnParticles(g, p->position, ORANGE, 12, 120.0f, 0.5f);
                TriggerScreenShake(g, 3.0f); /* T56 — splash patlama sarsıntısı */
                /* Splash'tan ölen tüm düşmanları işle */
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    Enemy *e = &g->enemies[j];
                    if (!e->active || e->currentHp > 0.0f)
                        continue;
                    SpawnParticles(g, e->position, e->color, 10, 100.0f, 0.4f);
                    int reward = KILL_REWARD;
                    if (e->type == ENEMY_FAST)
                        reward = (int)(KILL_REWARD * 1.2f);
                    if (e->type == ENEMY_TANK)
                        reward = (int)(KILL_REWARD * 2.0f);
                    g->gold += reward;
                    g->score += reward * 2;
                    g->enemiesKilled++;
                    EarnProsperity(&g->homeCity, 5);
                    e->active = false;
                    PlaySFX(&g->audio,
                            e->type == ENEMY_TANK ? SFX_TANK_DEATH : SFX_ENEMY_DEATH); /* T64 */
                }
            } else {
                target->currentHp -= p->damage;
                SpawnFloatingText(g, target->position, p->damage, false, false); /* T57 */
                SpawnParticles(g, p->position, p->color, 5, 80.0f, 0.2f);
                PlaySFX(&g->audio, p->sourceType == TOWER_SNIPER ? SFX_SHOOT_SNIPER
                                                                 : SFX_SHOOT_BASIC); /* T64 */
                /* Öldü mü? */
                if (target->currentHp <= 0.0f) {
                    SpawnParticles(g, target->position, target->color, 10, 100.0f, 0.4f);
                    int reward = KILL_REWARD;
                    if (target->type == ENEMY_FAST)
                        reward = (int)(KILL_REWARD * 1.2f);
                    if (target->type == ENEMY_TANK)
                        reward = (int)(KILL_REWARD * 2.0f);
                    g->gold += reward;
                    g->score += reward * 2;
                    g->enemiesKilled++;
                    EarnProsperity(&g->homeCity, 5);
                    target->active = false;
                    PlaySFX(&g->audio, target->type == ENEMY_TANK ? SFX_TANK_DEATH
                                                                  : SFX_ENEMY_DEATH); /* T64 */
                }
            }
            p->active = false;
        }
    }
}

/* ============================================================
 * T24 — UpdateParticles
 * ============================================================ */

/* Parçacıkları hareket ettirir, ömürlerini ve alfa değerlerini günceller. */
void UpdateParticles(Game *g, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &g->particles[i];
        if (!p->active)
            continue;
        p->position = Vec2Add(p->position, Vec2Scale(p->velocity, dt));
        p->lifetime -= dt;
        if (p->lifetime <= 0.0f) {
            p->active = false;
            continue;
        }
        float ratio = p->lifetime / p->maxLifetime;
        p->size = (4.0f + 4.0f) * ratio; /* Zamanla küçül */
        p->color.a = (unsigned char)(255.0f * ratio);
    }
}

/* ============================================================
 * T56 — ScreenShake
 * ============================================================ */

/* Sarsıntı başlatır; büyük patlama, level-up vb. olaylardan çağrılır. */

        void DrawParticles(Game * g) {
            for (int i = 0; i < MAX_PARTICLES; i++) {
                Particle *p = &g->particles[i];
                if (!p->active)
                    continue;
                DrawRectangle((int)(p->position.x - p->size / 2),
                              (int)(p->position.y - p->size / 2), (int)p->size, (int)p->size,
                              p->color);
            }
        }

        /* ============================================================
         * DrawPlacementPreview
         * ============================================================ */

        /* Fare grid üzerindeyken yerleştirilebilirliği gösteren yarı saydam önizleme çizer. */