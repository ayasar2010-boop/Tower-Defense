#ifndef UI_H
#define UI_H

/* ════════════════════════════════════════════════════════════════════
 * UI FRAMEWORK — Heroic Painterly Realism
 * Tipografi, panel, buton ve kaynak göstergesi sistemi.
 * ════════════════════════════════════════════════════════════════════ */

#include "raylib.h"
#include <stdbool.h>

/* ── Referans çözünürlük & ölçek ────────────────────────────────── */
#define UI_REF_WIDTH    1920.0f

/* ── Renk paleti ────────────────────────────────────────────────── */
#define UI_GOLD         ((Color){255, 215,   0, 255})
#define UI_GOLD_DIM     ((Color){160, 110,   0, 200})
#define UI_GOLD_BRIGHT  ((Color){255, 245, 100, 255})
#define UI_GOLD_GLOW    ((Color){255, 215,   0,  60})
#define UI_IVORY        ((Color){245, 245, 240, 255})
#define UI_OBSIDIAN     ((Color){ 18,  16,  24, 235})
#define UI_PANEL_TOP    ((Color){ 24,  28,  50, 245})
#define UI_PANEL_BOT    ((Color){  6,   6,  12, 255})
#define UI_BEVEL_HI     ((Color){ 80,  80, 110,  80})
#define UI_BEVEL_SH     ((Color){  0,   0,   0, 100})
#define UI_SHADOW_CLR   ((Color){  0,   0,   0, 155})
#define UI_SEPARATOR    ((Color){255, 215,   0, 100})

/* ── Buton animasyon durumu (caller tarafından saklanır) ─────────── */
typedef struct {
    float scale;      /* mevcut ölçek — lerp hedefi 1.0/1.05/0.97 */
    float glowAlpha;  /* hover ışıma alfa — lerp hedefi 0.0/1.0   */
} BtnAnim;

/* ── Global UI bağlamı ──────────────────────────────────────────── */
typedef struct {
    Font  title;        /* Başlık fontu: Cinzel/Trajan, 48pt */
    Font  body;         /* Gövde fontu: okunabilir serif, 24pt */
    bool  fontsLoaded;
    float uiScale;      /* screenWidth / UI_REF_WIDTH */
} UIContext;

extern UIContext g_ui;

/* ── Yaşam döngüsü ──────────────────────────────────────────────── */
void InitUI(int screenWidth);
void UnloadUI(void);

/* ── Ölçek inline yardımcıları ──────────────────────────────────── */
static inline float UIScale(void)   { return g_ui.uiScale; }
static inline float UIS(float v)    { return v * g_ui.uiScale; }
static inline int   UISI(float v)   { return (int)(v * g_ui.uiScale); }

/* ── İlkel yardımcılar ──────────────────────────────────────────── */
void DrawGradientPanel(Rectangle r, Color top, Color bottom);
void DrawOrnateBorder(Rectangle r, Color col, float thickness);
void DrawBevelRect(Rectangle r, Color fill, Color hilight, Color shadow, float bevel);

/* ── Tipografi ──────────────────────────────────────────────────── */
void DrawShadowText(Font font, const char *text, Vector2 pos,
                    float size, float spacing, Color col);
void DrawEpicTitle(const char *text, Vector2 pos, float size);
void DrawBodyText(const char *text, Vector2 pos, float size, Color col);

/* ── Bileşenler ─────────────────────────────────────────────────── */
void DrawEpicPanel(Rectangle bounds, const char *title);
bool DrawEpicButton(Rectangle bounds, const char *text, BtnAnim *anim, float dt);
void DrawResourceText(Vector2 pos, int value, const char *label, Color iconColor);

#endif
