# Tower Defense RPG — CLAUDE.md (AI Skillset Manual)

## Proje Özeti

C99 + Raylib ile yazılmış, üstten bakış (top-down) perspektifli **TD-RPG-Strategy Hybrid** oyunu.
Derleme: `make` (Windows: MSYS2 MinGW64 / Linux: gcc)

## Proje Yapısı

```text
TD_Test/
├── src/
│   ├── main.c          ← Ana oyun döngüsü + temel TD sistemleri
│   ├── homecity.c/h    ← CivLevel, ShipmentPoints, takviye kart sistemi
│   ├── siege.c/h       ← İzometrik koordinatlar, sur yapıları, birlik istasyonları
│   └── dungeon.c/h     ← Hero serbest hareket, Dungeon düşmanları, loot/envanter
├── assets/             ← Sprite/ses (şimdilik boş)
├── .vscode/
│   ├── extensions.json       ← Önerilen eklentiler
│   ├── settings.json         ← Editor + IntelliSense + Clang-Format
│   ├── c_cpp_properties.json ← Raylib IntelliSense yolları
│   ├── tasks.json            ← make / make clean / make run görevleri
│   └── launch.json           ← F5 → GDB debug (game.exe)
├── Makefile
├── CLAUDE.md           ← Bu dosya (AI bağlamı)
├── TASKS.md            ← Görev listesi
├── .gitignore
└── guidance.md         ← Teknik şartname / sistem prompt
```

## Teknik Kısıtlar

- **Dil**: Saf C (C99/C11), C++ yok
- **Kütüphane**: Raylib v4.5+ (MSYS2: `C:/msys64/mingw64/include`)
- **Derleme**: `make` — `src/*.c` tüm dosyaları derlenir (wildcard)
- **Çözünürlük**: 1280×720 @ 60 FPS
- **Bellek**: Sadece statik diziler — `malloc/free` yasak, `#define` limitler

## Build Sistemi (Makefile)

```bash
make          # Derle + DLL'leri kopyala (Windows)
make run      # Derle + çalıştır
make clean    # Temizle
```

Derleyici: `C:/msys64/mingw64/bin/gcc.exe -std=c99 -Wall -O2`
Bağlantı (Windows): `-lraylib -lopengl32 -lgdi32 -lwinmm`

## VS Code Hızlı Başvuru

| Kısayol          | Eylem                        |
|------------------|------------------------------|
| `Ctrl+Shift+B`   | Build: make (varsayılan)     |
| `F5`             | Debug: GDB ile başlat        |
| `Ctrl+F5`        | Run: debug'suz çalıştır      |

## Mimari & State Machine

```text
Game Loop: Input → Update → Draw
States:
  STATE_MENU → STATE_PLAYING → STATE_WAVE_CLEAR → STATE_GAMEOVER
                                                 → STATE_VICTORY
  STATE_PLAYING ↔ STATE_PAUSED
  STATE_PLAYING → STATE_DUNGEON  (T44 — planlanan)
  PREP_PHASE (T43 — planlanan, dalga arası inşa)
```

## Modül Bağımlılıkları

```text
main.c
  ├── homecity.h  → CivLevel, ShipmentPoints, Prosperity, takviye kartları
  ├── siege.h     → İzometrik koordinat matrisi, Wall Segment, Unit Stations
  └── dungeon.h   → Hero, DungeonEnemy, Loot, Inventory
```

## Temel TD Sistemleri (main.c)

| Sistem       | Notlar                                       |
|--------------|----------------------------------------------|
| Map/Grid     | 20×12, hardcoded, `InitMap()`                |
| Waypoints    | Pixel koordinatları, `< 4.0f` eşiği          |
| Enemy        | 3 tip: Normal/Fast/Tank, statik dizi         |
| Tower        | 3 tip: Basic/Sniper/Splash, upgrade sistemi  |
| Projectile   | Homing, splash hasar                         |
| Particle     | Alpha-fade, size shrink                      |
| Wave         | 10 dalga, progresif zorluk                   |
| HUD          | Üst panel, kule seçim, durum ekranları       |

## Önemli Sabitler

```c
GRID_COLS 20 / GRID_ROWS 12 / CELL_SIZE 48
MAX_ENEMIES 50 / MAX_TOWERS 30 / MAX_PROJECTILES 100
STARTING_GOLD 200 / STARTING_LIVES 20
```

## Kod Kuralları

- Fonksiyon isimleri: `PascalCase` (Raylib convention)
- Struct üyeleri: `camelCase`
- Global değişkenler: modül başlığında `extern` ile aç
- Her `.h` dosyası include guard kullan: `#ifndef MODULE_H`
- Her fonksiyon başına 1-2 satır Türkçe yorum
- Karmaşık algoritmalar adım adım açıklanmalı
- `malloc/free` YOK — statik diziler + `active` flag pattern

## Kontroller

| Girdi          | Eylem                               |
|----------------|-------------------------------------|
| Sol Tık        | Kule yerleştir                      |
| Sağ Tık        | Kule yükselt / sat (menü)           |
| 1 / 2 / 3      | Basic / Sniper / Splash seç         |
| P / ESC        | Duraklat                            |
| F              | Hız: 1× ↔ 2×                        |
| G              | Grid göster/gizle                   |
| SPACE          | Sonraki dalgayı başlat / Prep fazı  |
| D              | Dungeon moduna gir (WAVE_CLEAR'da)  |
| R              | Yeniden başlat                      |
| H              | Kamera hero takibi ac/kapat (T63)   |
| Scroll         | Kamera zoom (0.5x - 2.0x)           |
| Sağ Tık        | Hero hareketi — tıklanan noktaya pathfinding (Dungeon) |
| Sol Tık        | Hero yakın saldırısı (Dungeon modu)                    |
| Q              | Skill 1: Kılıç Darbesi — koni AOE (Dungeon)            |
| W              | Skill 2: Komuta Çığlığı — HP %20 iyileştir (Dungeon)   |
| E              | Skill 3: Sis Perdesi — 100px AOE yavaşlatma (Dungeon)  |
| R              | Skill 4: Destek Çağır — şehirlerden müttefik (Dungeon) |
| ESC            | Dungeon'dan çık (altın bonus aktar)                    |

## Modüller Arası Veri Akışı (T45)

```text
┌─────────────────────────────────────────────────────────────────┐
│                         STATE MAKİNESİ                          │
│                                                                 │
│  MENU → WORLD_MAP → LEVEL_BRIEFING → LOADING → PLAYING         │
│                                                    │            │
│                                         düşman ölümü           │
│                                            │                   │
│                                   ┌────────▼────────┐          │
│                                   │   HOME CITY      │          │
│                                   │  EarnProsperity  │          │
│                                   │  (+5 per kill)   │          │
│                                   └─────────────────┘          │
│                                                    │            │
│                                         dalga biter            │
│                                            │                   │
│                                   WAVE_CLEAR / PREP_PHASE       │
│                                      │           │             │
│                              [D] Dungeon    [SPACE] devam       │
│                                      │                         │
│                          ┌───────────▼───────────┐             │
│                          │     DUNGEON MODE       │             │
│                          │  Hero: WASD + SPACE    │             │
│                          │  Mobs → loot düşürür   │             │
│                          │  LOOT_GOLD →           │             │
│                          │    inventory.bonusGold  │             │
│                          │  LOOT_POTION/RUNE/GEAR │             │
│                          │    inventory sayaçları  │             │
│                          └───────────┬───────────┘             │
│                                      │ [ESC]                   │
│                          game.gold += bonusGold                 │
│                                      │                         │
│                              preDungeonState'e dön             │
│                         (WAVE_CLEAR veya PREP_PHASE)            │
└─────────────────────────────────────────────────────────────────┘
```

### Modüller Arası Veri Transfer Noktaları

| Kaynak                | Hedef       | Veri                        | Tetikleyici               |
|-----------------------|-------------|-----------------------------|---------------------------|
| Enemy (kill)          | HomeCity    | `EarnProsperity(+5)`        | Her düşman ölümünde       |
| HomeCity              | Game.gold   | Sevkiyat etkisi (planlanan) | ShipmentPoints dolunca    |
| DungeonMode.inventory | Game.gold   | `bonusGold` aktarımı        | ESC ile Dungeon'dan çıkış |
| Game.gold             | DungeonMode | —                           | (Dungeon kaynak tüketmez) |
| SiegeMechanics        | Game        | WallSegment hasar           | PREP_PHASE güncelleme     |

### Doğrulanmış Entegrasyon Noktaları (T45)

- **Home City ↔ TD**: `EarnProsperity` her düşman öldürülünce çalışır (main.c:1141, 1159)
- **TD → Dungeon**: `KEY_D` + `InitDungeon()` → temiz başlangıç, `preDungeonState` kaydedilir
- **Dungeon → TD**: `KEY_ESCAPE` → `game.gold += dungeon.inventory.bonusGold` → önceki state'e dön
- **PREP_PHASE**: `UpdateSiegeMechanics` + `UpdatePrepPhase` her frame çalışır, `prepTimer` sayar
- **UI Framework**: `InitUI(SCREEN_WIDTH)` → tüm paneller ölçekli çizim (`g_ui.uiScale`)
