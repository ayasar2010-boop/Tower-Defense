#ifndef HUD_H
#define HUD_H
#include "types.h"
void TriggerScreenShake(Game *g, float intensity);
void UpdateScreenShake(Game *g, float rawDt);
void SpawnFloatingText(Game *g, Vector2 pos, float value, bool isCrit, bool isHeal);
void UpdateFloatingTexts(Game *g, float dt);
void DrawFloatingTexts(Game *g);
void UpdateGameCamera(Game *g, float dt);
void DrawHUD(Game *g);
void DrawPlacementPreview(Game *g);
void DrawGame(Game *g);
void UpdateFogOfWar(Game *g);
void DrawMinimap(Game *g);
void DrawContextMenu(Game *g);
void DrawPathArrows(Game *g);
void DrawFriendlyUnits(Game *g);
void InitFriendlyUnit(FriendlyUnit *fu, FUnitType type, Vector2 pos);
void SpawnHeroUnit(Game *g);
void UpdateFriendlyUnits(Game *g, float dt);
bool IsButtonClicked(Rectangle btn);
void DrawButton(Rectangle btn, const char *label, Color bg, Color fg);
#endif
