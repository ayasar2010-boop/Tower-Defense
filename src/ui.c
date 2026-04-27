#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ════════════════════════════════════════════════════════════════════
 * UI FRAMEWORK — Heroic Painterly Realism
 * ════════════════════════════════════════════════════════════════════ */

UIContext g_ui = {0};

/* ── Font başlık boyutları (piksel, 1920 referans) ────────────────── */
#define FONT_TITLE_SIZE 48
#define FONT_BODY_SIZE  24

/* ── Drop shadow offset (referans piksel) ─────────────────────────── */
#define SHADOW_OFFSET 2.0f

/* ════════════════════════════════════════════════════════════════════
 * Yaşam döngüsü
 * ════════════════════════════════════════════════════════════════════ */

void InitUI(int screenWidth) {
    g_ui.uiScale = (float)screenWidth / UI_REF_WIDTH;
    if (g_ui.uiScale < 0.4f) g_ui.uiScale = 0.4f;  /* minimum ölçek koruması */

    int titlePx = (int)(FONT_TITLE_SIZE * g_ui.uiScale);
    int bodyPx  = (int)(FONT_BODY_SIZE  * g_ui.uiScale);
    if (titlePx < 12) titlePx = 12;
    if (bodyPx  < 10) bodyPx  = 10;

    /* Serif font yükleme — font dosyası yoksa Raylib varsayılanı */
    const char *titlePath = "assets/fonts/Cinzel-Regular.ttf";
    const char *bodyPath  = "assets/fonts/Bellefair-Regular.ttf";

    if (FileExists(titlePath))
        g_ui.title = LoadFontEx(titlePath, titlePx, NULL, 256);
    else
        g_ui.title = GetFontDefault();

    if (FileExists(bodyPath))
        g_ui.body = LoadFontEx(bodyPath, bodyPx, NULL, 256);
    else
        g_ui.body = GetFontDefault();

    g_ui.fontsLoaded = true;
}

void UnloadUI(void) {
    /* Yalnızca dosyadan yüklendiyse serbest bırak */
    if (g_ui.title.texture.id != GetFontDefault().texture.id)
        UnloadFont(g_ui.title);
    if (g_ui.body.texture.id  != GetFontDefault().texture.id)
        UnloadFont(g_ui.body);
    g_ui.fontsLoaded = false;
}

/* ════════════════════════════════════════════════════════════════════
 * İlkel yardımcılar
 * ════════════════════════════════════════════════════════════════════ */

/* Üstten alta doğru lineer gradient panel */
void DrawGradientPanel(Rectangle r, Color top, Color bottom) {
    DrawRectangleGradientV((int)r.x, (int)r.y,
                           (int)r.width, (int)r.height, top, bottom);
}

/* İşlemeli altın filigran çerçeve: 2px kenarlık + köşe elmas süsler */
void DrawOrnateBorder(Rectangle r, Color col, float thickness) {
    /* Ana kenarlık */
    DrawRectangleLinesEx(r, thickness, col);

    /* Köşe elmasları */
    float d    = UIS(7.0f);   /* elmas yarı-boyutu */
    float inset = thickness + UIS(3.0f);
    Vector2 corners[4] = {
        {r.x + inset,           r.y + inset},
        {r.x + r.width - inset, r.y + inset},
        {r.x + inset,           r.y + r.height - inset},
        {r.x + r.width - inset, r.y + r.height - inset}
    };
    Color dimCol = {col.r, col.g, col.b, (unsigned char)(col.a * 0.7f)};
    for (int i = 0; i < 4; i++) {
        /* Elmas = 4 üçgenden oluşan dörtgen */
        Vector2 c = corners[i];
        DrawTriangle((Vector2){c.x,     c.y - d},
                     (Vector2){c.x + d, c.y},
                     (Vector2){c.x,     c.y + d}, dimCol);
        DrawTriangle((Vector2){c.x,     c.y - d},
                     (Vector2){c.x,     c.y + d},
                     (Vector2){c.x - d, c.y}, dimCol);
    }

    /* Kenar orta noktaları: ince küçük çizgi süsleri */
    float mx = r.x + r.width  * 0.5f;
    float my = r.y + r.height * 0.5f;
    float tick = UIS(8.0f);
    Color tickCol = {col.r, col.g, col.b, (unsigned char)(col.a * 0.5f)};
    DrawLineEx((Vector2){mx - tick * 0.5f, r.y + thickness * 0.5f},
               (Vector2){mx + tick * 0.5f, r.y + thickness * 0.5f}, thickness, tickCol);
    DrawLineEx((Vector2){mx - tick * 0.5f, r.y + r.height - thickness * 0.5f},
               (Vector2){mx + tick * 0.5f, r.y + r.height - thickness * 0.5f}, thickness, tickCol);
    DrawLineEx((Vector2){r.x + thickness * 0.5f, my - tick * 0.5f},
               (Vector2){r.x + thickness * 0.5f, my + tick * 0.5f}, thickness, tickCol);
    DrawLineEx((Vector2){r.x + r.width - thickness * 0.5f, my - tick * 0.5f},
               (Vector2){r.x + r.width - thickness * 0.5f, my + tick * 0.5f}, thickness, tickCol);
}

/* Bevel: fill + üst-sol parlak kenar + alt-sağ gölge kenar */
void DrawBevelRect(Rectangle r, Color fill, Color hilight, Color shadow, float bevel) {
    DrawRectangleRec(r, fill);
    /* Üst kenar */
    DrawLineEx((Vector2){r.x,           r.y},
               (Vector2){r.x + r.width, r.y}, bevel, hilight);
    /* Sol kenar */
    DrawLineEx((Vector2){r.x, r.y},
               (Vector2){r.x, r.y + r.height}, bevel, hilight);
    /* Alt kenar */
    DrawLineEx((Vector2){r.x,           r.y + r.height},
               (Vector2){r.x + r.width, r.y + r.height}, bevel, shadow);
    /* Sağ kenar */
    DrawLineEx((Vector2){r.x + r.width, r.y},
               (Vector2){r.x + r.width, r.y + r.height}, bevel, shadow);
}

/* ════════════════════════════════════════════════════════════════════
 * Tipografi
 * ════════════════════════════════════════════════════════════════════ */

/* Drop shadow: önce gölge (+2px offset), sonra asıl metin */
void DrawShadowText(Font font, const char *text, Vector2 pos,
                    float size, float spacing, Color col) {
    float off = SHADOW_OFFSET * g_ui.uiScale;
    DrawTextEx(font, text,
               (Vector2){pos.x + off, pos.y + off},
               size, spacing, UI_SHADOW_CLR);
    DrawTextEx(font, text, pos, size, spacing, col);
}

/* Altın başlık metni — serif font, drop shadow */
void DrawEpicTitle(const char *text, Vector2 pos, float size) {
    float scaledSize = size * g_ui.uiScale;
    DrawShadowText(g_ui.title, text, pos, scaledSize, 1.5f, UI_GOLD);
}

/* Gövde metni — ivory rengi, shadow */
void DrawBodyText(const char *text, Vector2 pos, float size, Color col) {
    float scaledSize = size * g_ui.uiScale;
    DrawShadowText(g_ui.body, text, pos, scaledSize, 1.0f, col);
}

/* ════════════════════════════════════════════════════════════════════
 * DrawEpicPanel — Başlıklı obsidyen panel
 * ════════════════════════════════════════════════════════════════════ */

void DrawEpicPanel(Rectangle bounds, const char *title) {
    float bev = UIS(2.0f);

    /* 1. Gradient arka plan */
    DrawGradientPanel(bounds, UI_PANEL_TOP, UI_PANEL_BOT);

    /* 2. Bevel çerçeve */
    DrawBevelRect(bounds, (Color){0,0,0,0}, UI_BEVEL_HI, UI_BEVEL_SH, bev);

    /* 3. İşlemeli altın border */
    DrawOrnateBorder(bounds, UI_GOLD, UIS(1.5f));

    /* 4. Başlık (metin yoksa atla) */
    if (title && title[0] != '\0') {
        float titleSize = 18.0f * g_ui.uiScale;
        Vector2 measured = MeasureTextEx(g_ui.title, title, titleSize, 1.5f);
        Vector2 titlePos = {
            bounds.x + (bounds.width - measured.x) * 0.5f,
            bounds.y + UIS(10.0f)
        };
        DrawShadowText(g_ui.title, title, titlePos, titleSize, 1.5f, UI_GOLD);

        /* Başlık altı ayraç çizgisi */
        float sepY  = titlePos.y + measured.y + UIS(4.0f);
        float sepX1 = bounds.x + UIS(12.0f);
        float sepX2 = bounds.x + bounds.width - UIS(12.0f);
        DrawLineEx((Vector2){sepX1, sepY}, (Vector2){sepX2, sepY},
                   UIS(1.0f), UI_SEPARATOR);
    }
}

/* ════════════════════════════════════════════════════════════════════
 * DrawEpicButton — Fantastik stilde animasyonlu buton
 * ════════════════════════════════════════════════════════════════════ */

/* Buton animasyonu varsayılan başlangıç değerlerini ayarlar */
static void InitBtnAnimIfNeeded(BtnAnim *anim) {
    /* Hiç dokunulmamış (sıfırlanmış) animasyonu başlat */
    if (anim->scale < 0.1f) anim->scale = 1.0f;
}

bool DrawEpicButton(Rectangle bounds, const char *text, BtnAnim *anim, float dt) {
    InitBtnAnimIfNeeded(anim);

    Vector2 mouse   = GetMousePosition();
    bool    hover   = CheckCollisionPointRec(mouse, bounds);
    bool    pressed = hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool    clicked = hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    /* ── Animasyon: ölçek lerp ────────────────────────────────────── */
    float targetScale = pressed ? 0.97f : (hover ? 1.05f : 1.0f);
    float t = fminf(dt * 14.0f, 1.0f);
    anim->scale     += (targetScale     - anim->scale)     * t;
    anim->glowAlpha += ((hover ? 1.0f : 0.0f) - anim->glowAlpha) * fminf(dt * 10.0f, 1.0f);

    /* ── Ölçeklenmiş sınır (merkeze göre) ───────────────────────────── */
    Rectangle sb = {
        bounds.x + bounds.width  * (1.0f - anim->scale) * 0.5f,
        bounds.y + bounds.height * (1.0f - anim->scale) * 0.5f,
        bounds.width  * anim->scale,
        bounds.height * anim->scale
    };

    /* ── 1. Dış ışıma (hover glow) ──────────────────────────────── */
    if (anim->glowAlpha > 0.01f) {
        for (int ring = 3; ring >= 1; ring--) {
            float expand = (float)ring * UIS(4.0f);
            Color gc = UI_GOLD_GLOW;
            gc.a = (unsigned char)(UI_GOLD_GLOW.a * anim->glowAlpha * ((4 - ring) / 3.0f));
            Rectangle gr = {
                sb.x - expand, sb.y - expand,
                sb.width  + expand * 2.0f,
                sb.height + expand * 2.0f
            };
            DrawRectangleLinesEx(gr, UIS(1.0f), gc);
        }
    }

    /* ── 2. Gradient fill ─────────────────────────────────────────── */
    Color top, bot;
    if (pressed) {
        top = (Color){ 80,  55,   0, 230};
        bot = (Color){ 40,  28,   0, 255};
    } else if (hover) {
        top = (Color){200, 155,  10, 235};
        bot = (Color){100,  75,   0, 255};
    } else {
        top = (Color){120,  90,   0, 220};
        bot = (Color){ 55,  40,   0, 245};
    }
    DrawGradientPanel(sb, top, bot);

    /* ── 3. Bevel ─────────────────────────────────────────────────── */
    float bev  = UIS(pressed ? 1.0f : 1.5f);
    Color hiC  = pressed ? UI_BEVEL_SH : UI_BEVEL_HI;
    Color shC  = pressed ? UI_BEVEL_HI : UI_BEVEL_SH;
    DrawBevelRect(sb, (Color){0,0,0,0}, hiC, shC, bev);

    /* ── 4. Altın kenarlık ────────────────────────────────────────── */
    Color borderCol = hover ? UI_GOLD_BRIGHT : UI_GOLD;
    DrawRectangleLinesEx(sb, UIS(1.5f), borderCol);

    /* ── 5. Metin (drop shadow + merkez hizalı) ──────────────────── */
    float textSize = 16.0f * g_ui.uiScale * anim->scale;
    Vector2 measured = MeasureTextEx(g_ui.body, text, textSize, 1.0f);
    Vector2 textPos = {
        sb.x + (sb.width  - measured.x) * 0.5f,
        sb.y + (sb.height - measured.y) * 0.5f
    };
    Color textCol = hover ? UI_GOLD_BRIGHT : UI_IVORY;
    DrawShadowText(g_ui.body, text, textPos, textSize, 1.0f, textCol);

    return clicked;
}

/* ════════════════════════════════════════════════════════════════════
 * DrawResourceText — Altın / XP kaynak göstergesi
 * ════════════════════════════════════════════════════════════════════ */

void DrawResourceText(Vector2 pos, int value, const char *label, Color iconColor) {
    float s     = g_ui.uiScale;
    int   iSize = (int)(18.0f * s);   /* ikon kare boyutu */
    int   gap   = (int)(5.0f  * s);

    /* İkon kutusu */
    DrawRectangle((int)pos.x, (int)pos.y, iSize, iSize, iconColor);
    DrawRectangleLinesEx(
        (Rectangle){pos.x, pos.y, (float)iSize, (float)iSize},
        s, UI_GOLD);

    /* Değer metni (altın) */
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    float valSize = 15.0f * s;
    Vector2 valPos = {pos.x + iSize + gap, pos.y + (iSize - valSize) * 0.5f};
    DrawShadowText(g_ui.body, buf, valPos, valSize, 1.0f, UI_GOLD);

    /* Etiket metni (soluk ivory) */
    if (label && label[0] != '\0') {
        Vector2 valMeasured = MeasureTextEx(g_ui.body, buf, valSize, 1.0f);
        float   lblSize  = 11.0f * s;
        Color   lblColor = {245, 245, 240, 150};
        Vector2 lblPos = {
            valPos.x + valMeasured.x + gap,
            valPos.y + (valSize - lblSize) * 0.6f
        };
        DrawTextEx(g_ui.body, label, lblPos, lblSize, 1.0f, lblColor);
    }
}
