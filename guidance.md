# SYSTEM PROMPT: C & Raylib Tower Defense — Kapsamlı Mimari ve Tam Uygulama

Senden C programlama dilini kullanarak, üstten bakış (top-down) perspektifine sahip, tam oynanabilir
bir **Tower Defense (Kule Savunma)** oyunu için kapsamlı bir teknik iskelet ve çalışan başlangıç kodu
oluşturmanı istiyorum. Oyun, aşağıda tanımlanan tüm sistemleri içermeli ve Raylib kütüphanesi yüklü
bir sistemde `gcc main.c -lraylib -lm -o game && ./game` komutuyla doğrudan derlenip çalışabilmelidir.

---

## 1. TEKNİK GEREKSİNİMLER

| Alan               | Detay                                                                 |
|---------------------|-----------------------------------------------------------------------|
| **Dil**            | Saf C (C99 veya C11 standartları, C++ uzantısı yok)                  |
| **Kütüphane**      | Raylib (v4.5+) — grafik, girdi, ses alt yapısı                       |
| **Derleme**        | Tek dosya (`main.c`), bağımlılık: `-lraylib -lm -lpthread -ldl -lrt` |
| **Hedef Çözünürlük** | 1280×720 piksel, 60 FPS hedefli                                    |
| **Mimari**         | Struct-based modüler yapı, temiz Game Loop (Input → Update → Draw)   |
| **Bellek Yönetimi** | Statik diziler (malloc/free kullanma), `#define` ile sabit limitler  |

---

## 2. SABİTLER VE KONFIGÜRASYON (`#define`)

Oyunun tüm dengeleme parametreleri dosyanın başında tanımlanmalı:

```
#define SCREEN_WIDTH           1280
#define SCREEN_HEIGHT          720
#define GRID_COLS              20        // Harita sütun sayısı
#define GRID_ROWS              12        // Harita satır sayısı
#define CELL_SIZE              48        // Her grid hücresinin piksel boyutu (48×48)
#define GRID_OFFSET_X          ((SCREEN_WIDTH - GRID_COLS * CELL_SIZE) / 2)
#define GRID_OFFSET_Y          80        // Üst kısımda HUD için boşluk

#define MAX_ENEMIES            50
#define MAX_TOWERS             30
#define MAX_PROJECTILES        100
#define MAX_PARTICLES          200       // Görsel efektler için
#define MAX_WAYPOINTS          20
#define MAX_WAVES              10

#define ENEMY_BASE_SPEED       60.0f     // piksel/saniye
#define ENEMY_BASE_HP          100.0f

#define TOWER_COST_BASIC       50
#define TOWER_COST_SNIPER      100
#define TOWER_COST_SPLASH      150

#define STARTING_GOLD          200
#define STARTING_LIVES         20
#define KILL_REWARD            15
```

---

## 3. VERİ YAPILARI (Structs)

### 3.1 Enum Tanımları

```c
typedef enum { CELL_EMPTY = 0, CELL_PATH = 1, CELL_BUILDABLE = 2, CELL_TOWER = 3 } CellType;
typedef enum { TOWER_BASIC, TOWER_SNIPER, TOWER_SPLASH, TOWER_TYPE_COUNT } TowerType;
typedef enum { ENEMY_NORMAL, ENEMY_FAST, ENEMY_TANK, ENEMY_TYPE_COUNT } EnemyType;
typedef enum { STATE_MENU, STATE_PLAYING, STATE_PAUSED, STATE_WAVE_CLEAR, STATE_GAMEOVER, STATE_VICTORY } GameState;
```

### 3.2 Enemy (Düşman)

```c
typedef struct {
    Vector2 position;          // Dünya koordinatı (piksel)
    Vector2 direction;         // Normalize edilmiş hareket vektörü
    float speed;               // piksel/saniye
    float maxHp;
    float currentHp;
    float slowTimer;           // Yavaşlatma efekti süresi (saniye)
    float slowFactor;          // 0.0–1.0 arası yavaşlama oranı
    int currentWaypoint;       // Takip edilen waypoint indeksi
    EnemyType type;
    Color color;               // Düşman tipi rengi
    float radius;              // Çarpışma ve çizim yarıçapı
    bool active;
} Enemy;
```

### 3.3 Tower (Kule)

```c
typedef struct {
    Vector2 position;          // Grid merkez koordinatı (piksel)
    int gridX, gridY;          // Grid hücre koordinatları
    TowerType type;
    float range;               // Menzil (piksel)
    float damage;
    float fireRate;            // Atış/saniye
    float fireCooldown;        // Kalan bekleme süresi
    float splashRadius;        // TOWER_SPLASH için patlama yarıçapı (diğerleri 0)
    int level;                 // Yükseltme seviyesi (1–3)
    float rotation;            // Namlu yönü (derece, çizim için)
    int targetEnemyIndex;      // Hedef düşman dizin numarası (-1 = hedef yok)
    Color color;
    bool active;
} Tower;
```

### 3.4 Projectile (Mermi)

```c
typedef struct {
    Vector2 position;
    Vector2 velocity;          // Hız vektörü (piksel/saniye)
    float damage;
    float splashRadius;        // 0 ise splash yok
    float speed;               // Mermi hızı (piksel/saniye)
    int targetIndex;           // Hedef düşman indeksi
    TowerType sourceType;      // Hangi kule tipinden atıldı (görsel ayrım)
    Color color;
    bool active;
} Projectile;
```

### 3.5 Particle (Görsel Efekt)

```c
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;            // Kalan ömür (saniye)
    float maxLifetime;
    Color color;
    float size;
    bool active;
} Particle;
```

### 3.6 Wave (Dalga Sistemi)

```c
typedef struct {
    EnemyType enemyType;       // Bu dalgadaki düşman tipi
    int enemyCount;            // Toplam düşman sayısı
    float spawnInterval;       // İki düşman arası süre (saniye)
    float spawnTimer;          // Spawn zamanlayıcı
    int spawnedCount;          // Şimdiye dek spawn edilen
    float preDelay;            // Dalga başlamadan önceki bekleme (saniye)
    bool started;
} Wave;
```

### 3.7 Game (Ana Oyun Durumu)

```c
typedef struct {
    int grid[GRID_ROWS][GRID_COLS];         // Harita grid'i (CellType)
    Vector2 waypoints[MAX_WAYPOINTS];       // Düşman yol noktaları (piksel)
    int waypointCount;

    Enemy enemies[MAX_ENEMIES];
    Tower towers[MAX_TOWERS];
    Projectile projectiles[MAX_PROJECTILES];
    Particle particles[MAX_PARTICLES];
    Wave waves[MAX_WAVES];

    int currentWave;
    int totalWaves;
    bool waveActive;

    int gold;
    int lives;
    int score;
    int enemiesKilled;

    GameState state;
    TowerType selectedTowerType;            // Yerleştirmek için seçili kule tipi
    float gameSpeed;                        // 1.0 = normal, 2.0 = hızlı
    bool showGrid;                          // Grid çizgilerini göster/gizle
} Game;
```

---

## 4. OYUN HARİTASI VE WAYPOINT SİSTEMİ

### 4.1 Grid Düzeni
- 2D integer dizisi (`grid[GRID_ROWS][GRID_COLS]`) ile temsil et.
- Değerler: `CELL_EMPTY (0)`, `CELL_PATH (1)`, `CELL_BUILDABLE (2)`, `CELL_TOWER (3)`.
- Haritayı `InitMap(Game *game)` fonksiyonunda hardcode olarak tanımla.
- Yol, sol kenardan başlayıp sağ kenarda biten, en az 2-3 viraj içeren bir rota olmalı.
- Yol kenarlarındaki tüm hücreler `CELL_BUILDABLE` olarak işaretlenmeli.

### 4.2 Waypoint Sistemi
- Yolun dönüş noktaları `waypoints[]` dizisinde piksel koordinatı olarak saklanmalı.
- Düşmanlar `waypoints[0]`'dan `waypoints[waypointCount-1]`'e doğru ilerlemeli.
- Waypoint'e ulaşma mesafesi: `< 4.0f piksel` — bu eşik aşıldığında `currentWaypoint++`.
- Son waypoint'e ulaşan düşman → `lives--`, düşman deaktive edilir.

---

## 5. DÜŞMAN SİSTEMİ

### 5.1 Düşman Tipleri ve Özellik Tablosu

| Tip           | HP     | Hız Çarpanı | Yarıçap | Renk      | Ödül Çarpanı |
|---------------|--------|-------------|---------|-----------|---------------|
| ENEMY_NORMAL  | 100    | 1.0×        | 10      | RED       | 1.0×          |
| ENEMY_FAST    | 60     | 1.8×        | 8       | ORANGE    | 1.2×          |
| ENEMY_TANK    | 300    | 0.6×        | 14      | DARKPURPLE| 2.0×          |

### 5.2 Hareket Algoritması
```
Her frame için (dt = GetFrameTime()):
  1. Hedef waypoint'e yön vektörü hesapla:
       direction = Normalize(waypoints[currentWaypoint] - position)
  2. Yavaşlatma efektini uygula:
       effectiveSpeed = speed * (slowTimer > 0 ? slowFactor : 1.0)
  3. Pozisyonu güncelle:
       position += direction * effectiveSpeed * dt
  4. Hedefe ulaşıldı mı kontrol et:
       if (Distance(position, waypoints[currentWaypoint]) < 4.0f)
           currentWaypoint++
  5. Son waypoint aşıldıysa:
       lives--, enemy.active = false
```

### 5.3 Sağlık Çubuğu
- Her düşmanın üstünde yatay HP bar çiz (genişlik: `radius * 2.5`).
- Arkaplan: DARKGRAY, dolgu: yeşilden kırmızıya gradyan (HP oranına göre).
- Sadece `currentHp < maxHp` olduğunda göster.

---

## 6. KULE SİSTEMİ

### 6.1 Kule Tipleri ve Özellik Tablosu

| Tip          | Maliyet | Menzil | Hasar | Atış Hızı | Splash | Renk      |
|--------------|---------|--------|-------|-----------|--------|-----------|
| TOWER_BASIC  | 50      | 150px  | 20    | 2.0/s     | 0      | BLUE      |
| TOWER_SNIPER | 100     | 300px  | 80    | 0.5/s     | 0      | DARKGREEN |
| TOWER_SPLASH | 150     | 120px  | 30    | 1.0/s     | 60px   | MAROON    |

### 6.2 Hedef Seçim Algoritması (Targeting)
Kuleler "en yakın düşman" stratejisini kullanmalı:
```
int FindTarget(Game *game, Tower *tower):
    minDist = FLOAT_MAX
    targetIdx = -1
    for each active enemy:
        dist = Distance(tower->position, enemy.position)
        if (dist <= tower->range AND dist < minDist):
            minDist = dist
            targetIdx = i
    return targetIdx
```

### 6.3 Ateş Etme Mekanizması
```
Her frame (dt):
    tower->fireCooldown -= dt
    if (fireCooldown <= 0 AND target geçerli):
        CreateProjectile(tower->position, targetIndex, tower->damage, tower->splashRadius)
        fireCooldown = 1.0 / tower->fireRate
```

### 6.4 Kule Yerleştirme Kuralları
1. Fare pozisyonunu grid koordinatına çevir:
   `gridX = (mouseX - GRID_OFFSET_X) / CELL_SIZE`
   `gridY = (mouseY - GRID_OFFSET_Y) / CELL_SIZE`
2. Sınır kontrolü: `0 ≤ gridX < GRID_COLS` ve `0 ≤ gridY < GRID_ROWS`
3. Hücre kontrolü: `grid[gridY][gridX] == CELL_BUILDABLE`
4. Altın kontrolü: `gold >= towerCost`
5. Başarılıysa: hücreyi `CELL_TOWER` yap, altını düş, Tower struct'ı aktifleştir.

### 6.5 Kule Yükseltme (Upgrade)
- Mevcut kuleye sağ tık → seviye yükselt (maks 3).
- Her seviye: hasar ×1.3, menzil ×1.1, maliyet = başlangıç maliyeti × seviye.
- Görsel olarak kule boyutunu ve rengini koyulaştırarak seviyeleri ayırt et.

---

## 7. MERMİ (PROJECTILE) SİSTEMİ

### 7.1 Mermi Hareketi
```
Her frame (dt):
    if (hedef aktif):
        direction = Normalize(target.position - projectile.position)
        projectile.velocity = direction * projectile.speed
    position += velocity * dt
    if (Distance(position, target.position) < target.radius):
        → İsabet! Hasar uygula
        → splashRadius > 0 ise: Çevredeki tüm düşmanlara splash hasar uygula
        → Particle efekti oluştur
        → Mermiyi deaktive et
    if (hedef öldüyse):
        → Mermiyi deaktive et (veya son bilinen pozisyona ilerlet)
```

### 7.2 Splash Hasar
- TOWER_SPLASH mermileri isabet noktasında `splashRadius` içindeki tüm düşmanlara hasar verir.
- Splash hasarı mesafeye göre azalsın: `damage * (1.0 - dist/splashRadius)`.

---

## 8. DALGA (WAVE) SİSTEMİ

### 8.1 Dalga Tanımları
```c
void InitWaves(Game *game) {
    // Dalga 1: 8 normal düşman, 1.2 saniye aralıkla
    // Dalga 2: 12 normal + 3 hızlı karışık
    // Dalga 3: 6 tank düşman, 2 saniye aralıkla
    // ...progresif zorluk artışı
}
```

### 8.2 Dalga Akışı
1. `STATE_WAVE_CLEAR` durumunda "Sonraki Dalga" butonu veya 5 saniyelik otomatik başlatma.
2. Dalga aktifken: `spawnTimer` sayar, her aralıkta yeni düşman spawn eder.
3. Tüm düşmanlar öldürüldüğünde veya yolun sonuna ulaştığında → dalga tamamlandı.
4. Son dalga temizlendiğinde → `STATE_VICTORY`.

---

## 9. KULLANICI ARAYÜZÜ (HUD)

### 9.1 Üst Panel (y: 0–GRID_OFFSET_Y)
Sol taraf: `❤ Canlar: 20  |  💰 Altın: 200  |  ⚔ Skor: 0`
Sağ taraf: `Dalga: 1/10  |  Hız: 1x/2x  |  ⏸ Duraklat`

### 9.2 Alt Panel veya Yan Panel
- Kule seçim butonları (3 adet, kule tipi + maliyet bilgisi).
- Seçili kulenin özelliklerini gösteren tooltip.
- Fare grid üzerindeyken: yerleştirilebilirlik önizlemesi (yeşil/kırmızı yarı saydam kare).
- Seçili kulenin menzilini gösteren yarı saydam daire.

### 9.3 Oyun Durumu Ekranları
- **STATE_MENU**: Oyun başlığı + "Başla" butonu (basit dikdörtgen + metin).
- **STATE_PAUSED**: Yarı saydam karartma + "Devam Et" / "Çık" butonları.
- **STATE_GAMEOVER**: "GAME OVER" + skor + "Yeniden Başla" butonu.
- **STATE_VICTORY**: "TEBRİKLER!" + final skor + "Yeniden Başla" butonu.

---

## 10. PARTİKÜL (EFEKT) SİSTEMİ

### 10.1 Efekt Tetikleyicileri
- **Düşman ölümü**: 8–12 parçacık, düşman renginde, dışa doğru dağılır (lifetime: 0.4s).
- **Mermi isabet**: 4–6 parçacık, mermi renginde (lifetime: 0.2s).
- **Splash patlama**: 10–15 parçacık, turuncu/sarı, geniş dağılım (lifetime: 0.5s).

### 10.2 Parçacık Güncelleme
```
position += velocity * dt
lifetime -= dt
size *= (lifetime / maxLifetime)    // Zamanla küçülme
color.a = (unsigned char)(255 * (lifetime / maxLifetime))  // Solma
if (lifetime <= 0): active = false
```

---

## 11. GİRDİ YÖNETİMİ

| Girdi                    | Eylem                                          |
|--------------------------|-------------------------------------------------|
| Sol Tık (Grid üzerinde)  | Seçili kuleyi yerleştir                         |
| Sağ Tık (Kule üzerinde)  | Kuleyi yükselt (upgrade)                        |
| Tuş `1`, `2`, `3`        | Kule tipini seç (Basic, Sniper, Splash)         |
| Tuş `P` veya `ESCAPE`    | Oyunu duraklat / devam ettir                    |
| Tuş `F`                  | Oyun hızını değiştir (1× ↔ 2×)                 |
| Tuş `G`                  | Grid çizgilerini göster/gizle                   |
| Tuş `SPACE`              | Sonraki dalgayı başlat (wave clear durumunda)   |
| Tuş `R`                  | Oyunu yeniden başlat (game over / victory'de)   |

---

## 12. ÇİZİM KATMANLARI (Draw Order)

Çizim sırası (arkadan öne):

1. **Arkaplan**: Koyu yeşil/gri zemin rengi (`ClearBackground`)
2. **Grid Hücreleri**: Hücre tipine göre renk — yol: açık kahverengi, boş: koyu yeşil, buildable: hafif yeşil
3. **Grid Çizgileri**: İnce, yarı saydam çizgiler (opsiyonel, `showGrid` ile toggle)
4. **Menzil Önizleme**: Fare grid üzerindeyken seçili kulenin menzil dairesi (yarı saydam)
5. **Kuleler**: Kare gövde + yönlendirilmiş namlu çizgisi (rotation ile)
6. **Düşmanlar**: Daire (tip renginde) + sağlık çubuğu
7. **Mermiler**: Küçük daire veya çizgi (kule tipine göre renk)
8. **Parçacıklar**: Alpha blended küçük kareler/daireler
9. **HUD**: Üst panel, alt panel, butonlar, metinler
10. **Durum Ekranları**: Menu, Pause, GameOver, Victory overlay'leri

---

## 13. YARDIMCI FONKSİYONLAR

Aşağıdaki utility fonksiyonlarını implemente et:

```c
// Vektör işlemleri
float Vector2Distance(Vector2 a, Vector2 b);
Vector2 Vector2Normalize(Vector2 v);
Vector2 Vector2Subtract(Vector2 a, Vector2 b);
Vector2 Vector2Scale(Vector2 v, float scale);
Vector2 Vector2Add(Vector2 a, Vector2 b);
float Vector2Length(Vector2 v);

// Grid ↔ Dünya koordinat dönüşümleri
Vector2 GridToWorld(int gridX, int gridY);     // Grid hücresinin merkez piksel koordinatı
bool WorldToGrid(Vector2 worldPos, int *gx, int *gy);  // Piksel → grid indeks

// Oyun mantığı yardımcıları
void SpawnEnemy(Game *game, EnemyType type);
void CreateProjectile(Game *game, Vector2 origin, int targetIdx, float damage, float splash);
void SpawnParticles(Game *game, Vector2 pos, Color color, int count, float speed, float lifetime);
int FindNearestEnemy(Game *game, Vector2 pos, float range);
bool CanPlaceTower(Game *game, int gx, int gy);
int GetTowerCost(TowerType type);
```

---

## 14. ANA GAME LOOP YAPISI

```c
int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tower Defense - C & Raylib");
    SetTargetFPS(60);

    Game game = {0};
    InitGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime() * game.gameSpeed;

        switch (game.state) {
            case STATE_MENU:    UpdateMenu(&game);    break;
            case STATE_PLAYING: HandleInput(&game);
                                UpdateEnemies(&game, dt);
                                UpdateTowers(&game, dt);
                                UpdateProjectiles(&game, dt);
                                UpdateParticles(&game, dt);
                                UpdateWaves(&game, dt);
                                CheckGameConditions(&game);
                                break;
            case STATE_PAUSED:  UpdatePause(&game);   break;
            // ... diğer durumlar
        }

        BeginDrawing();
            ClearBackground((Color){34, 40, 49, 255});
            switch (game.state) {
                case STATE_MENU:    DrawMenu(&game);    break;
                case STATE_PLAYING: DrawGame(&game);
                                    DrawHUD(&game);     break;
                case STATE_PAUSED:  DrawGame(&game);
                                    DrawHUD(&game);
                                    DrawPauseOverlay(); break;
                // ... diğer durumlar
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

---

## 15. KOD KALİTESİ VE YORUM GEREKSİNİMLERİ

- **Her fonksiyonun** başına ne yaptığını açıklayan 1-2 satır Türkçe yorum yaz.
- **Karmaşık algoritmalar** (vektör hesaplamaları, hedef seçimi, splash hasar, grid dönüşümleri) adım adım açıkla.
- Sabit değerlerin yanına (ör. `< 4.0f`) neden o değerin seçildiğini açıkla.
- Fonksiyon isimlendirmesi: `PascalCase` (Raylib convention'a uygun).
- Struct üyeleri: `camelCase`.

---

## 16. ÇIKTI FORMATI

### 16.1 Önerilen Proje Yapısı
```
tower_defense/
├── src/
│   ├── main.c          ← Tüm oyun kodu (tek dosya)
│   └── (ileride bölünebilir: game.c, enemy.c, tower.c, ui.c, ...)
├── assets/             ← (Şimdilik boş, ileride sprite/ses dosyaları)
├── Makefile            ← Derleme komutu
└── README.md           ← Kurulum ve çalıştırma talimatları
```

### 16.2 Beklenen Çıktı
1. Yukarıdaki **tüm** struct tanımlarını, sabitleri, enum'ları ve fonksiyonları içeren tek bir `main.c`.
2. Kod derlendiğinde çalışan, oynanan bir oyun olmalı:
   - Harita ve yol görünür
   - Düşmanlar waypoint'leri takip ederek hareket eder
   - 1-2-3 tuşlarıyla kule seçilir, tıkla yerleştirilir
   - Kuleler ateş eder, mermiler hedefe gider, düşmanlar hasar alır
   - Can ve altın sistemi işler
   - Dalga sistemi çalışır
   - Game Over / Victory ekranları görüntülenir
3. Kodu olduğu gibi kopyalayıp derleyebileyim, ek dosya gerektirmesin.
4. Makefile örneği de sağla.

---

## 17. BONUS (ZORUNLU DEĞİL AMA İSTENİR)

- Kule satma (sağ tık menüsü, maliyetin %50'si geri döner).
- Düşman yolu üzerinde yön okları (görsel ipucu).
- Mini harita veya dalga ilerleme çubuğu.
- Düşmanların farklı geometrik şekillerle çizilmesi (normal: daire, hızlı: üçgen, tank: kare).
- Kule ateş ederken kısa bir "flash" animasyonu.
- Ses efektleri (`LoadSound` / `PlaySound` ile basit .wav dosyaları, yoksa atlansın).


## yeni özellikler 

# PROJECT EXPANSION: C/Raylib Tower Defense RPG Hybrid — Kapsamlı Genişletme Dokümanı

**Bağlam:** Temel Tower Defense mekanikleri (grid, düşman hareketi, waypoint takibi, 3 kule tipi,
mermi sistemi, dalga sistemi, partikül efektleri, HUD, game state machine) tamamlandı ve çalışır
durumda. Şimdi projeyi görsel olarak çok daha etkileyici, mekanik olarak derin ve tekrar oynanabilir
bir **Tower Defense + RPG Hibrit** oyununa dönüştürüyoruz.

**Hedef:** Oyuncunun yalnızca kule yerleştirmekle kalmayıp, bir kahramanı kontrol ettiği, seviye
atlayıp beceri seçtiği, boss savaşları yaptığı, envanter yönettiği ve hikaye diyalogları takip
ettiği katmanlı bir deneyim.

**Teknik Kısıt:** Saf C (C99/C11), Raylib v4.5+, modüler header/source ayrımı. Tüm kod
`gcc src/*.c -Iinclude -lraylib -lm -o game` komutuyla derlenebilmeli.

---

## 1. MODÜLER DOSYA MİMARİSİ

```
tower_defense_rpg/
├── include/
│   ├── config.h          ← Tüm sabitler, #define'lar, renk paletleri
│   ├── types.h           ← Tüm enum ve struct forward declaration'ları
│   ├── game.h            ← Game struct, InitGame, ResetGame
│   ├── hero.h            ← Hero sistemi, XP, Level, Stats, Skill Tree
│   ├── enemy.h           ← Enemy + Boss AI state machine
│   ├── tower.h           ← Tower, Upgrade, Synergy
│   ├── projectile.h      ← Projectile + Trail efekti
│   ├── inventory.h       ← Inventory, Item, Consumable
│   ├── wave.h            ← Wave, WaveManager, Boss Wave
│   ├── ui.h              ← HUD, Butonlar, Tooltip, Menüler
│   ├── dialogue.h        ← Dialogue sistemi, Portrait, TextBox
│   ├── map.h             ← Map, Interactable objeler, Tile animasyonları
│   ├── particle.h        ← Particle, ScreenShake, Efekt katmanları
│   ├── camera.h          ← Kamera sistemi (zoom, pan, smooth follow)
│   ├── audio.h           ← Ses yöneticisi (SFX + Müzik)
│   └── utils.h           ← Vektör math, Easing fonksiyonları, RNG
├── src/
│   ├── main.c            ← Entry point, ana döngü
│   ├── game.c            ← Game state machine, geçişler
│   ├── hero.c            ← Hero hareket, saldırı, skill kullanımı
│   ├── enemy.c           ← Enemy AI, Boss fazları
│   ├── tower.c           ← Tower yerleştirme, upgrade, synergy
│   ├── projectile.c      ← Mermi fizik, trail, homing
│   ├── inventory.c       ← Item ekleme/çıkarma, kullanım
│   ├── wave.c            ← Dalga spawn, boss tetikleme
│   ├── ui.c              ← Tüm UI çizim ve etkileşim
│   ├── dialogue.c        ← Diyalog parse, ilerleme, çizim
│   ├── map.c             ← Harita yükleme, interactable güncelleme
│   ├── particle.c        ← Partikül havuzu, efekt fabrikası
│   ├── camera.c          ← Kamera güncelleme, sınırlama
│   ├── audio.c           ← Ses yükleme, kanal yönetimi
│   └── utils.c           ← Yardımcı fonksiyon implementasyonları
├── assets/
│   ├── sounds/           ← .wav dosyaları (kule atış, patlama, level up, vb.)
│   └── data/             ← .txt diyalog dosyaları (ileride)
├── Makefile
└── README.md
```

### 1.1 Header Guard ve Include Zinciri

```c
// types.h — Tüm struct'lar burada, döngüsel bağımlılığı önler
#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"
#include "config.h"

// Forward declaration'lar ve tüm typedef'ler burada
// Diğer header'lar sadece types.h'ı include eder

#endif
```

**Kural:** Hiçbir `.h` dosyası başka bir proje `.h` dosyasını include etmez (sadece `types.h` ve
`config.h` hariç). Bu, derleme süresini kısa tutar ve döngüsel bağımlılığı engeller.

---

## 2. GENİŞLETİLMİŞ SABİTLER (`config.h`)

Önceki sabitlere ek olarak:

```c
// ─── Hero ───
#define HERO_BASE_HP           500.0f
#define HERO_BASE_MANA         200.0f
#define HERO_BASE_ATK          25.0f
#define HERO_BASE_DEF          10.0f
#define HERO_BASE_SPEED        120.0f    // piksel/saniye
#define HERO_MANA_REGEN        5.0f     // mana/saniye
#define HERO_XP_BASE           100      // 1. seviye için gereken XP
#define HERO_XP_MULTIPLIER     1.35f    // Her seviyede gereken XP çarpanı
#define HERO_MAX_LEVEL         20
#define HERO_ATTACK_RANGE      40.0f    // Otomatik saldırı menzili (piksel)
#define HERO_ATTACK_COOLDOWN   0.8f     // Saldırı hızı (saniye)
#define HERO_INVULN_TIME       0.5f     // Hasar aldıktan sonra dokunulmazlık

// ─── Inventory ───
#define INVENTORY_SLOTS        12       // 4×3 grid envanter
#define MAX_ITEM_TYPES         20
#define MAX_LOOT_DROPS         30

// ─── Boss ───
#define BOSS_HP_MULTIPLIER     8.0f     // Normal düşman HP × bu = Boss HP
#define BOSS_SIZE_MULTIPLIER   2.5f     // Görsel büyüklük çarpanı
#define BOSS_PHASE_COUNT       3        // Boss faz sayısı
#define BOSS_SPAWN_INTERVAL    10       // Her 10 dalgada bir boss

// ─── Dialogue ───
#define DIALOGUE_BOX_HEIGHT    140
#define DIALOGUE_PORTRAIT_SIZE 100
#define DIALOGUE_MAX_LINES     50
#define DIALOGUE_CHARS_PER_SEC 40       // Typewriter efekti hızı

// ─── Kamera ───
#define CAMERA_ZOOM_MIN        0.5f
#define CAMERA_ZOOM_MAX        2.0f
#define CAMERA_ZOOM_SPEED      0.1f
#define CAMERA_PAN_SPEED       300.0f
#define CAMERA_SMOOTH_FACTOR   8.0f     // Lerp hızı (yüksek = daha hızlı takip)

// ─── Görsel ───
#define TILE_ANIM_SPEED        2.0f     // Tile animasyon hızı
#define TRAIL_LENGTH           8        // Mermi kuyruk uzunluğu (pozisyon geçmişi)
#define SCREEN_SHAKE_DECAY     5.0f     // Sarsıntı sönümleme hızı
#define FLOATING_TEXT_SPEED    40.0f    // Hasar sayıları yükselme hızı
#define FLOATING_TEXT_LIFE     1.0f     // Hasar sayısı ömrü (saniye)

// ─── Synergy / Combo ───
#define SYNERGY_RADIUS         120.0f   // Kule sinerji etki alanı
#define COMBO_TIMEOUT          2.0f     // Arka arkaya kill süresi (saniye)

// ─── Renk Paleti (Tutarlı tema) ───
#define COLOR_BG               (Color){22, 27, 34, 255}      // Koyu lacivert arkaplan
#define COLOR_PATH             (Color){78, 68, 54, 255}      // Toprak yol
#define COLOR_PATH_BORDER      (Color){58, 50, 40, 255}      // Yol kenar çizgisi
#define COLOR_GRASS            (Color){34, 52, 34, 255}      // Koyu çimen
#define COLOR_BUILDABLE        (Color){42, 64, 42, 255}      // Kule konabilir alan
#define COLOR_GRID_LINE        (Color){255, 255, 255, 20}    // Çok hafif grid çizgisi
#define COLOR_HUD_BG           (Color){15, 18, 22, 220}      // HUD arka plan
#define COLOR_HUD_ACCENT       (Color){72, 145, 220, 255}    // Mavi vurgu
#define COLOR_GOLD_TEXT        (Color){255, 215, 0, 255}      // Altın sarısı
#define COLOR_HP_GREEN         (Color){76, 209, 55, 255}
#define COLOR_HP_RED           (Color){232, 65, 24, 255}
#define COLOR_MANA_BLUE        (Color){72, 126, 255, 255}
#define COLOR_XP_PURPLE        (Color){190, 120, 255, 255}
#define COLOR_CRITICAL         (Color){255, 50, 50, 255}     // Kritik hasar rengi
#define COLOR_HEAL             (Color){50, 255, 120, 255}    // İyileşme rengi
```

---

## 3. HERO (KAHRAMAN) SİSTEMİ

### 3.1 Enum ve Struct Tanımları

```c
typedef enum {
    HERO_WARRIOR,       // Yüksek HP, yakın dövüş, kule hasar bonusu
    HERO_MAGE,          // Yüksek Mana, AoE büyüler, kule menzil bonusu
    HERO_ARCHER,        // Dengeli, uzak saldırı, kule atış hızı bonusu
    HERO_CLASS_COUNT
} HeroClass;

typedef enum {
    HERO_IDLE,
    HERO_MOVING,
    HERO_ATTACKING,
    HERO_CASTING,       // Skill kullanırken
    HERO_HURT,          // Hasar animasyonu
    HERO_DEAD
} HeroState;

typedef struct {
    float hp, maxHp;
    float mana, maxMana;
    float atk;
    float def;
    float speed;
    float attackRange;
    float attackSpeed;      // Saldırı/saniye
    float critChance;       // 0.0–1.0
    float critMultiplier;   // Varsayılan: 2.0
    float manaRegen;        // Mana/saniye
} HeroStats;

typedef struct {
    char name[32];
    char description[128];
    int manaCost;
    float cooldown;
    float currentCooldown;
    float duration;         // Buff süresi (0 = anlık)
    float value;            // Hasar / iyileşme miktarı / buff değeri
    float radius;           // AoE yarıçapı (0 = tek hedef)
    int level;              // Skill seviyesi (1–5)
    int maxLevel;
    bool isPassive;         // true = otomatik etki, false = aktif kullanım
    bool unlocked;
    int requiredHeroLevel;  // Açılma seviyesi
    Color color;            // Skill ikonu rengi
} Skill;

#define MAX_SKILLS 8

typedef struct {
    Vector2 position;           // Dünya koordinatı
    Vector2 velocity;           // Mevcut hareket vektörü
    Vector2 targetPos;          // Hedef pozisyon (sağ tık ile verilen)
    HeroClass heroClass;
    HeroState state;
    HeroStats baseStats;        // Seviye başına temel değerler
    HeroStats currentStats;     // Buff / debuff sonrası aktif değerler
    Skill skills[MAX_SKILLS];
    int skillCount;
    int selectedSkill;          // Aktif seçili skill indeksi (-1 = yok)

    int level;
    int xp;
    int xpToNext;               // Sonraki seviye için gereken XP

    float attackCooldown;        // Saldırı zamanlayıcısı
    float invulnTimer;           // Dokunulmazlık süresi
    float animTimer;             // Animasyon zamanlayıcısı
    float bodyAngle;             // Vücut yönü (derece)

    int comboCount;              // Arka arkaya kill sayısı
    float comboTimer;            // Combo süresi

    // Görsel
    float bobOffset;             // Yürüme sırasında yukarı-aşağı salınım
    float scale;                 // 1.0 normal, hasar alınca kısa süre büyür
    Color bodyColor;
    Color accentColor;           // Sınıf vurgu rengi

    bool active;
} Hero;
```

### 3.2 Kahraman Sınıf Başlangıç Değerleri

| Özellik        | Warrior           | Mage             | Archer            |
|----------------|-------------------|------------------|--------------------|
| HP / MaxHP     | 700 / 700         | 350 / 350        | 450 / 450          |
| Mana / MaxMana | 80 / 80           | 300 / 300        | 150 / 150          |
| Atk            | 40                | 15               | 30                 |
| Def            | 20                | 5                | 10                 |
| Speed          | 100 px/s          | 90 px/s          | 130 px/s           |
| Attack Range   | 35 px (yakın)     | 120 px (büyü)    | 160 px (ok)        |
| Attack Speed   | 1.5/s             | 0.8/s            | 2.0/s              |
| Crit Chance    | 0.10              | 0.05             | 0.20               |
| Crit Multi     | 2.0               | 3.0              | 1.8                |
| Mana Regen     | 2.0/s             | 8.0/s            | 4.0/s              |
| Vücut Rengi    | SKYBLUE           | VIOLET           | LIME               |
| Kule Bonusu    | +15% hasar        | +20% menzil      | +25% atış hızı     |

### 3.3 Kahraman Hareket Mekanizması

```
Sağ tık ile hedef pozisyon verilir (yol veya boş alan, CELL_PATH veya CELL_BUILDABLE):
  1. targetPos = tıklanan dünya koordinatı
  2. state = HERO_MOVING
  3. Her frame:
     direction = Normalize(targetPos - position)
     velocity = direction * currentStats.speed
     position += velocity * dt
     bodyAngle = Atan2(direction.y, direction.x) * RAD2DEG  // Yüz yönü
     bobOffset = sin(animTimer * 10.0f) * 2.0f              // Yürüme salınımı
     if (Distance(position, targetPos) < 3.0f):
         state = HERO_IDLE
         velocity = {0, 0}
```

### 3.4 Kahraman Saldırı Sistemi

```
Otomatik saldırı (en yakın düşman menzildeyse):
  1. target = FindNearestEnemy(position, currentStats.attackRange)
  2. if (target != -1 AND attackCooldown <= 0):
     - Hasar hesapla:
       baseDmg = currentStats.atk
       isCrit = (RandomFloat(0,1) < currentStats.critChance)
       finalDmg = isCrit ? baseDmg * currentStats.critMultiplier : baseDmg
       finalDmg -= enemy.armor * 0.5f   // Zırh azaltması
     - Sınıfa göre saldırı:
       WARRIOR → Yakın dövüş ark efekti (yakın AoE 30px)
       MAGE    → Büyü mermisi (homing, yavaş ama güçlü)
       ARCHER  → Ok mermisi (hızlı, düz çizgi)
     - attackCooldown = 1.0 / currentStats.attackSpeed
     - isCrit ise: FloatingText "CRIT!" + ScreenShake(2.0f)
  3. Cooldown güncelle: attackCooldown -= dt
```

### 3.5 XP ve Seviye Atlama

```c
void AddXP(Hero *hero, int amount) {
    hero->xp += amount;
    while (hero->xp >= hero->xpToNext && hero->level < HERO_MAX_LEVEL) {
        hero->xp -= hero->xpToNext;
        hero->level++;
        hero->xpToNext = (int)(HERO_XP_BASE * powf(HERO_XP_MULTIPLIER, hero->level - 1));

        // Stat artışı (sınıfa göre farklı oranlar)
        hero->baseStats.maxHp += (hero->heroClass == HERO_WARRIOR) ? 40 : 20;
        hero->baseStats.maxMana += (hero->heroClass == HERO_MAGE) ? 25 : 10;
        hero->baseStats.atk += (hero->heroClass == HERO_ARCHER) ? 5 : 3;
        hero->baseStats.def += (hero->heroClass == HERO_WARRIOR) ? 4 : 2;
        hero->baseStats.hp = hero->baseStats.maxHp;  // Tam can iyileştirme
        hero->baseStats.mana = hero->baseStats.maxMana;

        // Efektler
        SpawnParticles(pos, COLOR_XP_PURPLE, 20, 80.0f, 0.8f);
        ScreenShake(3.0f);
        PlaySFX(SFX_LEVEL_UP);
        // Yeni skill açıldıysa → bildirim göster
    }
}
```

### 3.6 Skill Tree Sistemi

Her sınıf için 4 aktif + 2 pasif skill tanımla:

**Warrior Skill'leri:**

| Skill           | Tip    | Seviye | Mana | Cooldown | Açıklama                                        |
|-----------------|--------|--------|------|----------|--------------------------------------------------|
| Shield Bash     | Aktif  | 1      | 20   | 5s       | Yakındaki düşmanları iterek 2s stun              |
| War Cry         | Aktif  | 3      | 40   | 15s      | 8s boyunca çevredeki kulelere +30% hasar         |
| Iron Skin       | Pasif  | 2      | —    | —        | Kalıcı def +25%                                  |
| Earthquake      | Aktif  | 5      | 80   | 25s      | Geniş AoE hasar + 3s yavaşlatma                 |
| Last Stand      | Pasif  | 7      | —    | —        | HP %25 altındayken atk ve def ikiye katlanır     |
| Blade Storm     | Aktif  | 10     | 100  | 30s      | 5s boyunca çevresinde sürekli AoE hasar döndürür |

**Mage Skill'leri:**

| Skill           | Tip    | Seviye | Mana | Cooldown | Açıklama                                         |
|-----------------|--------|--------|------|----------|---------------------------------------------------|
| Fireball        | Aktif  | 1      | 25   | 4s       | Tek hedefe yüksek hasar + 3s yanma efekti        |
| Frost Nova      | Aktif  | 3      | 50   | 12s      | Çevredeki tüm düşmanları 4s %50 yavaşlat         |
| Arcane Mind     | Pasif  | 2      | —    | —        | Mana regen +40%                                   |
| Chain Lightning | Aktif  | 5      | 60   | 10s      | 4 düşmana zıplayan şimşek, her zıplamada %20 azalır |
| Mana Shield     | Pasif  | 7      | —    | —        | Hasarın %30'u mana'dan kesilir (HP yerine)        |
| Meteor Strike   | Aktif  | 10     | 120  | 35s      | Hedef noktaya 2s sonra düşen dev AoE hasar        |

**Archer Skill'leri:**

| Skill           | Tip    | Seviye | Mana | Cooldown | Açıklama                                          |
|-----------------|--------|--------|------|----------|---------------------------------------------------|
| Multi Shot      | Aktif  | 1      | 15   | 3s       | 3 ok aynı anda, her biri farklı hedefe            |
| Trap            | Aktif  | 3      | 30   | 10s      | Yere tuzak kur, basan düşman 3s stun + hasar      |
| Eagle Eye       | Pasif  | 2      | —    | —        | Kritik şansı +15%                                 |
| Rain of Arrows  | Aktif  | 5      | 70   | 18s      | Geniş alanda 3s boyunca ok yağmuru                |
| Evasion         | Pasif  | 7      | —    | —        | %20 saldırıdan kaçınma şansı                     |
| Phantom Arrow   | Aktif  | 10     | 90   | 25s      | Tüm düşmanlardan geçen delici ok, sınırsız menzil |

### 3.7 Skill Kullanım Mekanizması

```
Tuş Q, W, E, R → skills[0..3] aktif skill'leri tetikler
Tuş bağlama: İlk 4 skill = Q W E R, pasifler otomatik

Skill kullanım akışı:
  1. Tuş basıldı → selectedSkill = ilgili indeks
  2. Mana kontrolü: currentStats.mana >= skill.manaCost ?
  3. Cooldown kontrolü: skill.currentCooldown <= 0 ?
  4. AoE skill ise → fare pozisyonunda hedef dairesi göster → tıkla onayla
  5. Tek hedef ise → otomatik en yakın düşmana uygula
  6. Mana düş, cooldown başlat, efektleri uygula
  7. Skill çizim: daire dalga, şimşek çizgisi, patlama vb.
```

---

## 4. ENVANTER (INVENTORY) SİSTEMİ

### 4.1 Item Yapıları

```c
typedef enum {
    ITEM_NONE,
    ITEM_POTION_HP,         // Anlık HP iyileştirme
    ITEM_POTION_MANA,       // Anlık Mana iyileştirme
    ITEM_SCROLL_DAMAGE,     // 15s tüm kulelere +25% hasar
    ITEM_SCROLL_SPEED,      // 15s tüm kulelere +40% atış hızı
    ITEM_SCROLL_RANGE,      // 15s tüm kulelere +30% menzil
    ITEM_BOMB,              // Tıklanan noktada büyük AoE hasar
    ITEM_FREEZE_SCROLL,     // 5s tüm düşmanları dondur
    ITEM_GOLD_MAGNET,       // 20s boyunca altın kazancı ×2
    ITEM_SHIELD_RUNE,       // 10s hero'ya hasar kalkanı
    ITEM_RESURRECT_STONE,   // Hero ölünce otomatik diriliş (tek kullanım)
    ITEM_TYPE_COUNT
} ItemType;

typedef struct {
    ItemType type;
    char name[32];
    char description[80];
    int quantity;           // Stok adedi (stackable)
    int maxStack;           // Maksimum yığın (5–10 arası)
    int buyCost;            // Mağazadan alma maliyeti
    float value;            // Etki miktarı (HP miktarı, süre, hasar vb.)
    float duration;         // Buff süresi (0 = anlık)
    Color color;            // Envanter ikonu rengi
    int hotkey;             // 0 = atanmamış, 1-6 = sayı tuşları
} Item;

typedef struct {
    Item slots[INVENTORY_SLOTS];
    int selectedSlot;       // -1 = seçili değil
    bool isOpen;            // Envanter paneli açık mı
} Inventory;
```

### 4.2 Envanter UI Düzeni

```
Envanter paneli (I tuşu ile aç/kapat):
  ┌──────────────────────────────────────┐
  │  ENVANTER                     [X]    │
  │  ┌────┐ ┌────┐ ┌────┐ ┌────┐        │
  │  │ 1  │ │ 2  │ │ 3  │ │ 4  │        │  Satır 1
  │  │ HP │ │ MP │ │ ⚔  │ │ 💣 │        │
  │  │ x5 │ │ x3 │ │ x1 │ │ x2 │        │
  │  └────┘ └────┘ └────┘ └────┘        │
  │  ┌────┐ ┌────┐ ┌────┐ ┌────┐        │
  │  │ 5  │ │ 6  │ │    │ │    │        │  Satır 2
  │  │ ❄  │ │ 🛡 │ │    │ │    │        │
  │  │ x1 │ │ x1 │ │    │ │    │        │
  │  └────┘ └────┘ └────┘ └────┘        │
  │  ┌────┐ ┌────┐ ┌────┐ ┌────┐        │
  │  │    │ │    │ │    │ │    │        │  Satır 3
  │  └────┘ └────┘ └────┘ └────┘        │
  │                                      │
  │  Seçili: HP Potion (+150 HP)         │  Tooltip
  │  Hotkey: [1]    Stok: 5              │
  └──────────────────────────────────────┘

- Sol tık = kullan / seç
- Sağ tık = hotkey ata (1-6 arası sayı tuşu)
- Sayı tuşları (Shift + 1-6) = hızlı kullanım (envanter kapalıyken de çalışır)
- Shift basılıyken 1-6 tuşları item, basılı değilken kule seçimi
```

### 4.3 Item Loot Drop Sistemi

```c
typedef struct {
    Vector2 position;
    ItemType type;
    float lifetime;         // Yerden kaybolma süresi (15s)
    float bobTimer;         // Yukarı-aşağı salınım animasyonu
    float magnetRange;      // Hero bu mesafeye gelince otomatik toplar (60px)
    bool active;
} LootDrop;
```

```
Düşman öldüğünde:
  dropChance = 0.15 (Normal), 0.25 (Fast), 0.40 (Tank), 1.0 (Boss)
  if (RandomFloat(0,1) < dropChance):
      randomItem = ağırlıklı rastgele seçim (HP pot daha sık, Resurrect çok nadir)
      SpawnLootDrop(enemy.position, randomItem)

Loot toplama:
  Her frame → LootDrop'lar güncellenir:
    if (Distance(hero.position, loot.position) < loot.magnetRange):
        loot.position = Lerp(loot.position, hero.position, 8.0f * dt)  // Mıknatıs
    if (Distance < 10.0f):
        AddToInventory(hero.inventory, loot.type)
        SpawnParticles(pos, loot.color, 6, 50.0f, 0.3f)
        PlaySFX(SFX_PICKUP)
        loot.active = false
```

---

## 5. BOSS SİSTEMİ (State Machine Tabanlı)

### 5.1 Boss Struct

```c
typedef enum {
    BOSS_PHASE_1,       // Normal saldırı
    BOSS_PHASE_2,       // HP %60 altında — yeni yetenekler
    BOSS_PHASE_3,       // HP %25 altında — enrage modu
    BOSS_DEFEATED
} BossPhase;

typedef enum {
    BOSS_IDLE,
    BOSS_MOVING,
    BOSS_ATTACKING,
    BOSS_SPECIAL_ATTACK, // Özel yetenek kullanıyor
    BOSS_SUMMONING,      // Minion çağırıyor
    BOSS_VULNERABLE,     // Kısa süre ekstra hasar alır
    BOSS_ENRAGED         // Hız ve hasar artmış
} BossState;

typedef struct {
    // Enemy struct'ın tüm alanlarını içerir (embedded struct veya ayrı)
    Enemy base;                 // Temel düşman verileri
    BossPhase phase;
    BossState aiState;
    float stateTimer;           // Mevcut AI state süresi
    float specialCooldown;      // Özel saldırı bekleme süresi
    float summonCooldown;       // Minion çağırma bekleme süresi

    // Faz geçiş eşikleri
    float phase2Threshold;      // 0.60 (HP oranı)
    float phase3Threshold;      // 0.25

    // Görsel
    float pulseTimer;           // Nabız animasyonu
    float shieldAlpha;          // Kalkan görsel opaklığı
    Color phaseColor;           // Faza göre değişen renk
    float bodyRotation;         // Gövde dönüşü (görsel)

    int minionCount;            // Aktif minion sayısı
    bool hasShield;             // Faz geçişinde kısa süreli kalkan
    bool defeated;
} Boss;
```

### 5.2 Boss AI State Machine

```
BossUpdate(Boss *boss, Game *game, float dt):

  // ─── Faz geçiş kontrolü ───
  hpRatio = boss->base.currentHp / boss->base.maxHp
  if (hpRatio <= phase3Threshold AND phase != PHASE_3):
      phase = PHASE_3
      aiState = BOSS_ENRAGED
      boss->base.speed *= 1.5
      boss->base.color = RED
      hasShield = true → 3s boyunca hasar almaz
      ScreenShake(8.0f)
      SpawnParticles(pos, RED, 30, 120.0f, 1.0f)
      ShowDialogue("Boss", "YETER! Artık acımayacağım!")

  else if (hpRatio <= phase2Threshold AND phase != PHASE_2):
      phase = PHASE_2
      ShowDialogue("Boss", "İlginç... Gücümün gerçek halini göstereceğim.")

  // ─── AI State davranışları ───
  switch (aiState):

    case BOSS_MOVING:
        // Normal düşman gibi waypoint takibi (ama daha yavaş)
        MoveAlongPath(boss, dt)
        // Her 5 saniyede özel saldırı
        specialCooldown -= dt
        if (specialCooldown <= 0):
            aiState = BOSS_SPECIAL_ATTACK
            stateTimer = 1.5f // Saldırı animasyon süresi

    case BOSS_SPECIAL_ATTACK:
        // Faza göre farklı saldırılar:
        PHASE_1 → Düz çizgide yüksek hasarlı mermi (4 yöne)
        PHASE_2 → Etrafında dönen 8 mermi halkası
        PHASE_3 → Hedef alana 1.5s sonra düşen meteor (uyarı dairesi göster)
        stateTimer -= dt
        if (stateTimer <= 0):
            aiState = BOSS_MOVING
            specialCooldown = (phase == PHASE_3) ? 3.0f : 5.0f

    case BOSS_SUMMONING:
        // 2-4 minion düşman spawn et (Boss pozisyonundan)
        // Minion'lar normal düşman gibi waypoint takip eder
        // Her 15 saniyede bir tetiklenir (PHASE_2+)
        SpawnMinions(game, boss->base.position, 2 + phase)
        summonCooldown = 15.0f
        aiState = BOSS_MOVING

    case BOSS_VULNERABLE:
        // PHASE_2'de kalkan kırıldığında → 3s ekstra hasar alır
        // Görsel: düşman yanıp söner
        stateTimer -= dt
        if (stateTimer <= 0):
            aiState = BOSS_MOVING

    case BOSS_ENRAGED:
        // PHASE_3: Sürekli hızlı hareket, sürekli mermi yağmuru
        MoveAlongPath(boss, dt * 1.3f)
        // 0.8s'de bir 360° mermi
        specialCooldown -= dt
        if (specialCooldown <= 0):
            SpawnRadialProjectiles(boss->base.position, 12, 200.0f, 30.0f)
            specialCooldown = 0.8f
```

### 5.3 Boss Görsel Çizimi

```
DrawBoss(Boss *boss):
  // Nabız efekti (boyut artıp azalır)
  pulseScale = 1.0 + sin(pulseTimer * 3.0) * 0.05

  // Dış hale / glow efekti
  DrawCircleV(pos, radius * 1.8 * pulseScale, Fade(phaseColor, 0.15))
  DrawCircleV(pos, radius * 1.4 * pulseScale, Fade(phaseColor, 0.25))

  // Ana gövde (altıgen veya büyük daire)
  DrawPoly(pos, 6, radius * pulseScale, bodyRotation, phaseColor)
  DrawPolyLines(pos, 6, radius * pulseScale, bodyRotation, WHITE)

  // İç detay — faza göre sembol
  PHASE_1 → İç daire
  PHASE_2 → İç üçgen (dönüyor)
  PHASE_3 → İç yıldız (yanıp sönüyor)

  // Kalkan aktifse
  if (hasShield):
      DrawCircleLinesV(pos, radius * 2.0, Fade(SKYBLUE, shieldAlpha))

  // HP bar (ekstra geniş, faz renkleriyle)
  DrawBossHPBar(boss)

  // Boss ismi (üstte)
  DrawText("GOLEM KRALІ", pos.x - 60, pos.y - radius - 30, 16, WHITE)
```

---

## 6. DİYALOG SİSTEMİ

### 6.1 Struct Yapısı

```c
typedef struct {
    char speaker[32];           // Konuşan karakter adı
    char text[256];             // Diyalog metni
    Color speakerColor;         // İsim rengi
    int portraitType;           // 0=Hero, 1=Boss, 2=NPC, 3=System
} DialogueLine;

typedef struct {
    DialogueLine lines[DIALOGUE_MAX_LINES];
    int lineCount;
    int currentLine;

    // Typewriter efekti
    float charTimer;            // Karakter zamanlayıcısı
    int visibleChars;           // Şu ana dek görünen karakter sayısı
    bool lineComplete;          // Mevcut satır tamamen göründü mü

    // Görsel
    float boxY;                 // Kutu y pozisyonu (animasyonlu açılma)
    float targetBoxY;           // Hedef y pozisyonu
    float portraitBob;          // Konuşurken portre salınımı
    bool active;
} DialogueSystem;
```

### 6.2 Diyalog Tetikleyicileri

```
Diyalog tetikleme noktaları:
  - Oyun başlangıcı → Giriş hikayesi (2-3 satır)
  - Her 5. dalga öncesi → Kahraman veya NPC yorumu
  - Boss ortaya çıkınca → Boss tanıtım diyaloğu
  - Boss faz geçişi → Boss provokasyon repliği
  - Boss yenildiğinde → Zafer diyaloğu
  - Oyuncu ilk kule yerleştirdiğinde → Tutorial ipucu
  - Hazine sandığı açıldığında → "Bir şey buldun!" bildirimi
  - Game Over → Son söz
```

### 6.3 Typewriter Efekti ve Çizim

```
UpdateDialogue(DialogueSystem *ds, float dt):
  if (!ds->active) return;

  // Kutu açılma animasyonu (alt kenardan yukarı kayar)
  ds->boxY = Lerp(ds->boxY, ds->targetBoxY, 6.0f * dt)

  // Typewriter — her frame karakter ekle
  if (!ds->lineComplete):
      ds->charTimer += dt
      while (ds->charTimer >= 1.0f / DIALOGUE_CHARS_PER_SEC):
          ds->charTimer -= 1.0f / DIALOGUE_CHARS_PER_SEC
          ds->visibleChars++
          if (ds->visibleChars >= strlen(currentLine.text)):
              ds->lineComplete = true
              break

  // SPACE tuşu
  if (IsKeyPressed(KEY_SPACE)):
      if (!ds->lineComplete):
          ds->visibleChars = strlen(currentLine.text)  // Hemen tamamla
          ds->lineComplete = true
      else:
          ds->currentLine++  // Sonraki satır
          if (ds->currentLine >= ds->lineCount):
              ds->active = false  // Diyalog bitti
              game->state = STATE_PLAYING
          else:
              ds->visibleChars = 0
              ds->lineComplete = false

DrawDialogue(DialogueSystem *ds):
  // Arka plan kutusu (alt ekran, yarı saydam siyah)
  DrawRectangle(0, ds->boxY, SCREEN_WIDTH, DIALOGUE_BOX_HEIGHT, Fade(BLACK, 0.85))
  DrawRectangleLines(0, ds->boxY, SCREEN_WIDTH, DIALOGUE_BOX_HEIGHT, COLOR_HUD_ACCENT)

  // Portre (sol tarafta geometrik sembol — sınıfa göre)
  // Hero → kalkan şekli, Boss → altıgen, NPC → daire
  DrawPortrait(currentLine.portraitType, 30, ds->boxY + 20)

  // Konuşan ismi
  DrawText(currentLine.speaker, 140, ds->boxY + 15, 20, currentLine.speakerColor)

  // Metin (typewriter — sadece visibleChars kadar)
  char visibleText[256] = {0};
  strncpy(visibleText, currentLine.text, ds->visibleChars);
  DrawText(visibleText, 140, ds->boxY + 45, 18, RAYWHITE)

  // Devam göstergesi (metin tamamsa yanıp sönen üçgen)
  if (ds->lineComplete):
      float blink = (sinf(GetTime() * 5.0f) > 0) ? 1.0f : 0.3f
      DrawText("▼ SPACE", SCREEN_WIDTH - 120, ds->boxY + DIALOGUE_BOX_HEIGHT - 25, 14,
               Fade(WHITE, blink))
```

---

## 7. HARİTA ETKİLEŞİMLERİ (INTERACTABLE OBJECTS)

### 7.1 Struct Tanımı

```c
typedef enum {
    INTERACT_TREASURE,      // Hazine sandığı — rastgele item verir
    INTERACT_BRIDGE,        // Kırık köprü — tamir edince kısa yol açılır
    INTERACT_SHRINE,        // Sunak — altın karşılığı geçici buff
    INTERACT_TRAP_SLOT,     // Tuzak yuvası — Archer trap skill'i burada daha güçlü
    INTERACT_HEALING_WELL,  // İyileştirme kuyusu — 3 kullanım hakkı, hero'yu iyileştirir
    INTERACT_TYPE_COUNT
} InteractableType;

typedef struct {
    Vector2 position;           // Dünya koordinatı
    int gridX, gridY;           // Grid hücre pozisyonu
    InteractableType type;
    bool used;                  // Kullanıldı mı (sandık açıldı, köprü tamir edildi)
    int usesLeft;               // Kalan kullanım hakkı (iyileşme kuyusu: 3)
    int cost;                   // Etkileşim maliyeti (shrine: altın, diğerleri: 0)
    float interactRange;        // Hero bu mesafe içindeyken etkileşebilir (50px)
    float glowTimer;            // Animasyon: parlama efekti
    float value;                // Etki miktarı (HP, buff çarpanı vb.)
    char tooltip[80];           // Yaklaşınca gösterilecek açıklama
    Color color;
    bool active;
} Interactable;

#define MAX_INTERACTABLES 10
```

### 7.2 Etkileşim Akışı

```
Her frame:
  for each active interactable:
      dist = Distance(hero.position, interactable.position)
      if (dist < interactable.interactRange):
          // Tooltip göster: "E - Aç" / "E - Tamir Et (50 Altın)"
          DrawTooltip(interactable.tooltip, interactable.position)

          if (IsKeyPressed(KEY_E)):
              switch (type):
                  TREASURE → OpenTreasure(game, interactable)
                              // 1-2 rastgele item spawn et
                              // Sandık açılma animasyonu
                              // used = true

                  BRIDGE   → if (gold >= cost):
                                 gold -= cost
                                 // Yeni kısa yol waypoint'leri aktifleştir
                                 // Köprü görselini "tamir edilmiş" olarak değiştir
                                 used = true

                  SHRINE   → if (gold >= cost):
                                 gold -= cost
                                 // 30s boyunca hero'ya buff
                                 // (+20% atk VEYA +30% speed, rastgele)
                                 used = true

                  HEALING_WELL → if (usesLeft > 0):
                                     hero.currentStats.hp += value
                                     Clamp(hp, 0, maxHp)
                                     usesLeft--
                                     SpawnParticles(pos, COLOR_HEAL, 10, 40.0f, 0.5f)
```

---

## 8. GELİŞMİŞ GÖRSEL SİSTEMLER

### 8.1 Screen Shake (Ekran Sarsıntısı)

```c
typedef struct {
    float intensity;        // Mevcut şiddet (piksel)
    float maxIntensity;     // Başlangıç şiddeti
    float decay;            // Sönümleme hızı
    Vector2 offset;         // Her frame rastgele ofset
} ScreenShake;

// Güncelleme:
shake.intensity -= shake.decay * dt;
if (shake.intensity > 0):
    shake.offset.x = RandomFloat(-shake.intensity, shake.intensity)
    shake.offset.y = RandomFloat(-shake.intensity, shake.intensity)
else:
    shake.offset = (Vector2){0, 0}

// Kullanım:
BeginDrawing();
    // Tüm oyun çizimlerini offset ile kaydır
    DrawGame_WithOffset(shake.offset);
EndDrawing();
```

### 8.2 Floating Damage Numbers (Uçan Hasar Sayıları)

```c
typedef struct {
    Vector2 position;
    float value;            // Gösterilecek sayı
    float lifetime;
    float maxLifetime;
    Color color;            // Normal hasar: WHITE, Kritik: GOLD, İyileşme: GREEN
    bool isCrit;            // Kritik ise büyük font + "!" ekle
    float scale;            // Font büyüklüğü çarpanı (kritik = 1.5)
    bool active;
} FloatingText;

#define MAX_FLOATING_TEXTS 40

// Güncelleme:
position.y -= FLOATING_TEXT_SPEED * dt    // Yukarı hareket
lifetime -= dt
alpha = (lifetime / maxLifetime) * 255    // Solma
scale *= (1.0 + (1.0 - lifetime/maxLifetime) * 0.3)  // Hafif büyüme

// Çizim:
fontSize = (int)(16 * ft.scale)
if (ft.isCrit):
    DrawText(TextFormat("%.0f!", ft.value), pos.x, pos.y, fontSize + 4, Fade(COLOR_CRITICAL, alpha))
else:
    DrawText(TextFormat("%.0f", ft.value), pos.x, pos.y, fontSize, Fade(ft.color, alpha))
```

### 8.3 Mermi Trail (Kuyruk) Efekti

```c
// Projectile struct'ına ekle:
Vector2 trailPositions[TRAIL_LENGTH];
int trailIndex;

// Her frame:
projectile.trailPositions[projectile.trailIndex] = projectile.position;
projectile.trailIndex = (projectile.trailIndex + 1) % TRAIL_LENGTH;

// Çizim — kuyruk noktaları azalan opaklık ve boyutla:
for (int i = 0; i < TRAIL_LENGTH; i++):
    int idx = (trailIndex - i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH
    float t = (float)i / TRAIL_LENGTH            // 0 = en yeni, 1 = en eski
    float alpha = (1.0f - t) * 0.6f
    float radius = projectileRadius * (1.0f - t * 0.7f)
    DrawCircleV(trailPositions[idx], radius, Fade(projectile.color, alpha))
```

### 8.4 Tile Animasyonları

```
Yol tile'ları:
  - Kenar çizgileri çiz (her hücrenin komşu boş hücrelerine bakan kenarında)
  - Yolda hareket eden küçük noktalar (yürüyüş yönünü gösteren animasyon):
    dotOffset = fmod(gameTime * TILE_ANIM_SPEED, 1.0f)
    Her 3 tile'da bir, yol üzerinde küçük bir nokta akışı

Su tile'ları (opsiyonel):
  - Mavi rengin alpha'sı sinüs ile değişir
  - Küçük dalgacık çizgileri

Kule tabanları:
  - Kulenin oturduğu hücrede hafif glow
  - Saldırırken taban kısa süre parlak
```

### 8.5 Kule Gelişmiş Çizimi

```
DrawTower(Tower *tower):
  center = tower->position
  baseSize = CELL_SIZE * 0.35

  // Seviyeye göre taban detayı
  switch (tower->level):
      1 → Düz kare
      2 → Kare + iç çizgi
      3 → Kare + yıldız detay + dış glow

  // Kule tipi çizimi
  TOWER_BASIC:
      DrawRectanglePro(center, baseSize, baseSize, rotation, tower->color)
      // Namlu: ince uzun dikdörtgen (rotation yönünde)
      DrawLineEx(center, namluUcu, 3.0f, DARKBLUE)

  TOWER_SNIPER:
      // İnce uzun dikdörtgen gövde + çok uzun namlu
      // Menzil göstergesi: noktalı daire
      DrawCircleLinesV(center, tower->range, Fade(DARKGREEN, 0.15))

  TOWER_SPLASH:
      // Yuvarlak gövde (yıkıcı görüntü)
      DrawCircleV(center, baseSize * 0.6, tower->color)
      // Namlu: kısa kalın boru

  // Ateş flash'ı (fireCooldown sıfırlandığı anda kısa parıltı)
  if (tower->fireCooldown > (1.0/tower->fireRate - 0.05f)):
      DrawCircleV(namluUcu, 6, Fade(YELLOW, 0.8))

  // Menzil dairesi (fare üzerindeyken)
  if (mouseOverTower):
      DrawCircleLinesV(center, tower->range, Fade(WHITE, 0.3))
```

### 8.6 Düşman Geometrik Çeşitlilik

```
DrawEnemy(Enemy *enemy):
  switch (enemy->type):
      ENEMY_NORMAL:
          DrawCircleV(pos, radius, color)
          // İç daire (göz efekti)
          DrawCircleV(pos, radius * 0.4, Fade(WHITE, 0.3))

      ENEMY_FAST:
          // Üçgen (ilerleme yönüne dönük)
          DrawPoly(pos, 3, radius, directionAngle, color)
          // Hız çizgileri (arkasında 2-3 kısa çizgi)
          for i in 0..2:
              lineStart = pos - direction * (radius + 4 + i*5)
              DrawLineEx(lineStart, lineStart - direction*6, 1.5, Fade(color, 0.3 - i*0.1))

      ENEMY_TANK:
          // Kare (büyük, ağır görünüm)
          DrawRectanglePro(pos, radius*1.8, radius*1.8, directionAngle, color)
          // İç çapraz çizgiler (zırh efekti)
          DrawLineEx(...)  // X şeklinde çizgiler

  // Yavaşlatma efekti aktifse
  if (enemy->slowTimer > 0):
      DrawCircleV(pos, radius + 3, Fade(SKYBLUE, 0.25))   // Buz halkası
      // Küçük buz parçacıkları (1-2 adet, etrafında döner)

  // Hasar aldığında kısa yanıp sönme
  if (enemy->hitFlashTimer > 0):
      DrawCircleV(pos, radius, Fade(WHITE, 0.5))
```

---

## 9. KULE SİNERJİ (COMBO) SİSTEMİ

### 9.1 Sinerji Kuralları

```
İki kule SYNERGY_RADIUS (120px) içinde yanyana yerleştirildiğinde özel bonus:

| Kule 1       | Kule 2       | Sinerji Bonusu                           |
|-------------|-------------|-------------------------------------------|
| BASIC       | BASIC       | Her ikisine +15% atış hızı               |
| BASIC       | SNIPER      | Sniper'a +10% hasar                       |
| BASIC       | SPLASH      | Splash yarıçapı +20%                      |
| SNIPER      | SNIPER      | Her ikisine +15% menzil                   |
| SNIPER      | SPLASH      | Splash'a +20% hasar, Sniper'a +10% hız   |
| SPLASH      | SPLASH      | Her ikisine +25% splash yarıçapı          |

Sinerji tespiti — kule yerleştirildiğinde:
  for each other active tower:
      if (Distance(newTower.pos, otherTower.pos) <= SYNERGY_RADIUS):
          ApplySynergyBonus(newTower, otherTower)
          // Görsel: iki kule arası ince parlayan çizgi çiz (sinerji bağı)

Çizim:
  for each synergy pair:
      DrawLineEx(tower1.pos, tower2.pos, 1.5f, Fade(COLOR_HUD_ACCENT, 0.25 + sin(time*2)*0.1))
```

---

## 10. GELİŞMİŞ KAMERA SİSTEMİ

### 10.1 Struct

```c
typedef struct {
    Camera2D raylibCam;         // Raylib'in Camera2D'si
    Vector2 targetPos;          // Hedef odak noktası
    float targetZoom;           // Hedef zoom seviyesi
    float currentZoom;
    bool followHero;            // Hero'yu otomatik takip et
    Vector2 panOffset;          // Manuel kaydırma ofseti
    Vector2 bounds;             // Harita sınırları (kamera taşmasın)
} GameCamera;
```

### 10.2 Kamera Kontrolleri

```
Scroll Wheel → Zoom in/out (0.5× – 2.0×)
Orta fare butonu sürükle → Manuel pan
H tuşu → Hero'ya odaklan (followHero toggle)
Boşluk çubuğu (dalgalar arasında) → Haritaya genel bakış

Güncelleme:
  if (followHero):
      targetPos = Lerp(targetPos, hero.position, CAMERA_SMOOTH_FACTOR * dt)
  currentZoom = Lerp(currentZoom, targetZoom, CAMERA_SMOOTH_FACTOR * dt)

  // Sınır kontrolü — kamera harita dışına çıkmasın
  ClampCameraToMap(&camera)

  // ScreenShake ofseti kameraya uygulanır
  raylibCam.offset = screenCenter + shakeOffset
  raylibCam.target = targetPos + panOffset
  raylibCam.zoom = currentZoom
```

---

## 11. GELİŞMİŞ UI SİSTEMİ

### 11.1 Tıklanabilir Buton Altyapısı

```c
typedef struct {
    Rectangle rect;
    char text[32];
    Color normalColor;
    Color hoverColor;
    Color pressColor;
    Color currentColor;         // Animasyonlu geçiş
    bool isHovered;
    bool isPressed;
    float hoverScale;           // Hover'da hafif büyüme (1.0 → 1.05)
    float pressAnim;            // Basma animasyonu (tıklanınca küçülme)
} UIButton;

bool UpdateButton(UIButton *btn, Vector2 mousePos) {
    btn->isHovered = CheckCollisionPointRec(mousePos, btn->rect);
    btn->isPressed = btn->isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    // Renk geçişi (lerp ile yumuşak)
    Color target = btn->isHovered
        ? (IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? btn->pressColor : btn->hoverColor)
        : btn->normalColor;
    btn->currentColor = LerpColor(btn->currentColor, target, 10.0f * GetFrameTime());

    // Boyut animasyonu
    btn->hoverScale = Lerp(btn->hoverScale,
        btn->isHovered ? 1.05f : 1.0f, 8.0f * GetFrameTime());

    return btn->isPressed;
}
```

### 11.2 HUD Düzeni (Detaylı)

```
Üst Panel (y: 0–70, tam genişlik):
  ┌──────────────────────────────────────────────────────────────────┐
  │  ❤ 18/20    💰 350    ⚔ 1250    │  Dalga 5/10  │  [1×][2×][4×] [⏸]  │
  │  ┃████████░░┃                    │  ▓▓▓▓▓▓░░░░  │                     │
  │  Can barı                        │  Dalga ilerleme│  Hız butonları     │
  └──────────────────────────────────────────────────────────────────┘

Hero Paneli (sol alt köşe, 200×100):
  ┌─────────────────────┐
  │ [Portre]  Warrior   │
  │           Lv. 7     │
  │ HP ████████░░  500  │
  │ MP ██████░░░░  120  │
  │ XP ███░░░░░░░  35%  │
  │ Q⬡ W⬡ E⬡ R⬡       │  ← Skill slotları (cooldown overlay ile)
  └─────────────────────┘

Kule Seçim Paneli (alt orta):
  ┌──────────────────────────────────────┐
  │  [1] Basic    [2] Sniper   [3] Splash │
  │   50💰         100💰        150💰      │
  │  ─────────  ─────────   ─────────     │
  │  Seçili: ██  (menzil: 150, hasar: 20) │
  └──────────────────────────────────────┘

Envanter Kısa Çubuğu (sağ alt, 6 slot):
  ┌─────────────────────────┐
  │ [Shift+1] [Shift+2] ... [Shift+6] │
  │   HP×5     MP×3     💣×1           │
  └─────────────────────────┘

Sağ tık ile seçilen kulenin bilgi paneli:
  ┌────────────────────┐
  │  ◆ Sniper Kule     │
  │  Seviye: 2/3       │
  │  Hasar: 104 (+24)  │
  │  Menzil: 330       │
  │  Hız: 0.55/s       │
  │  ──────────────     │
  │  [Yükselt: 200💰]  │
  │  [Sat: 75💰]       │
  └────────────────────┘
```

### 11.3 Pause Menüsü

```
game.state == STATE_PAUSED olduğunda:
  // Arkaplanı karart
  DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6))

  // Merkez panel (animasyonlu açılma — scale 0→1)
  ┌─────────────────────┐
  │                     │
  │   ⏸ DURAKLATILDI    │
  │                     │
  │   [ ▶ Devam Et  ]   │
  │   [ ⚙ Ayarlar   ]   │
  │   [ ✕ Çıkış     ]   │
  │                     │
  └─────────────────────┘

ESC → PAUSED arasında toggle (ESC artık oyunu kapatmaz!)
"Devam Et" butonu veya ESC → STATE_PLAYING'e dön
"Çıkış" → STATE_MENU'ye dön (onay isteyebilir)
```

---

## 12. SES SİSTEMİ

### 12.1 Ses Enum'ları

```c
typedef enum {
    SFX_TOWER_PLACE,
    SFX_TOWER_SHOOT,
    SFX_TOWER_UPGRADE,
    SFX_TOWER_SELL,
    SFX_ENEMY_HIT,
    SFX_ENEMY_DEATH,
    SFX_BOSS_ROAR,
    SFX_BOSS_PHASE,
    SFX_HERO_ATTACK,
    SFX_HERO_HURT,
    SFX_HERO_LEVEL_UP,
    SFX_SKILL_CAST,
    SFX_ITEM_PICKUP,
    SFX_ITEM_USE,
    SFX_UI_CLICK,
    SFX_UI_HOVER,
    SFX_WAVE_START,
    SFX_WAVE_COMPLETE,
    SFX_GOLD_EARN,
    SFX_GAME_OVER,
    SFX_VICTORY,
    SFX_COUNT
} SFXType;
```

### 12.2 Ses Yöneticisi

```c
typedef struct {
    Sound effects[SFX_COUNT];
    Music bgMusic;
    float sfxVolume;        // 0.0–1.0
    float musicVolume;      // 0.0–1.0
    bool sfxEnabled;
    bool musicEnabled;
} AudioManager;

// Ses dosyaları yoksa sessizce atla (crash etme!)
void PlaySFX(AudioManager *audio, SFXType type) {
    if (audio->sfxEnabled && audio->effects[type].frameCount > 0) {
        SetSoundVolume(audio->effects[type], audio->sfxVolume);
        PlaySound(audio->effects[type]);
    }
}
```

**Not:** Ses dosyaları opsiyonel. Dosya bulunamazsa `LoadSound()` hata dönecek,
bu durumu kontrol edip sessiz modda çalışmaya devam et. Oyun ses dosyası olmadan
da tam işlevsel olmalı.

---

## 13. GELİŞMİŞ DALGA SİSTEMİ

### 13.1 Genişletilmiş Wave Struct

```c
typedef struct {
    EnemyType enemyType;
    int enemyCount;
    float spawnInterval;
    float spawnTimer;
    int spawnedCount;
    float preDelay;
    bool started;

    // Yeni alanlar:
    float enemyHpMultiplier;    // Dalga ilerledikçe HP çarpanı
    float enemySpeedMultiplier; // Dalga ilerledikçe hız çarpanı
    bool isBossWave;            // Bu dalgada boss var mı
    int bonusGold;              // Dalga tamamlama ödülü
    char waveName[32];          // "Dalga 5: Sürat Akını"
} Wave;
```

### 13.2 Dalga Örnekleri (20 Dalga)

```
Dalga  1: "İlk Akın"        → 8 Normal,   aralık 1.5s,  HP×1.0
Dalga  2: "Yol Keşfi"       → 10 Normal,  aralık 1.2s,  HP×1.1
Dalga  3: "Hızlı İzciler"   → 6 Fast,     aralık 1.0s,  HP×1.0
Dalga  4: "Karışık Güçler"  → 8 Normal + 4 Fast (karışık), HP×1.2
Dalga  5: "Zırhlı Öncüler"  → 4 Tank,     aralık 2.5s,  HP×1.0
Dalga  6: "Dalga Dalgası"   → 15 Normal,  aralık 0.8s,  HP×1.3
Dalga  7: "Hız Fırtınası"   → 12 Fast,    aralık 0.6s,  HP×1.2
Dalga  8: "Demir Duvar"     → 6 Tank + 6 Normal,         HP×1.4
Dalga  9: "Son Hazırlık"    → 10 Normal + 6 Fast + 3 Tank, HP×1.5
Dalga 10: "★ GOLEM KRAL ★"  → 1 BOSS + 4 Normal minion,  BOSS WAVE
Dalga 11: "Sessiz Tehdit"   → 20 Normal,  aralık 0.5s,  HP×1.7
Dalga 12: "Gölge Koşucular" → 15 Fast,    aralık 0.4s,  HP×1.5
Dalga 13: "Ağır Tugay"      → 8 Tank,     aralık 1.5s,  HP×1.6
Dalga 14: "Kaos Dalgası"    → 12 her tipten karışık,     HP×1.8
Dalga 15: "Cehennem Geçidi" → 25 Normal + 10 Fast,       HP×2.0
Dalga 16: "Tank Filosu"     → 12 Tank,    aralık 1.0s,  HP×2.0
Dalga 17: "Sel Baskını"     → 30 Fast,    aralık 0.3s,  HP×1.8
Dalga 18: "Elit Birlikleri" → 8 Tank + 8 Fast + 8 Normal, HP×2.2
Dalga 19: "Son Savunma"     → 20 her tipten + yüksek HP,  HP×2.5
Dalga 20: "★ KARANLIR LORD ★" → 1 FINAL BOSS + sürekli minion, BOSS WAVE
```

---

## 14. GİRDİ HARİTASI (GENİŞLETİLMİŞ)

| Girdi                        | Eylem                                              |
|------------------------------|------------------------------------------------------|
| Sol Tık (grid üzerinde)      | Kule yerleştir / Interactable seç                   |
| Sağ Tık (boş alan)           | Hero'yu hareket ettir                               |
| Sağ Tık (kule üzerinde)      | Kule bilgi panelini aç (yükselt / sat)              |
| Sağ Tık (düşman üzerinde)    | Hero'yu o düşmana saldırmaya yönlendir              |
| `1`, `2`, `3`                | Kule tipini seç                                     |
| `Shift + 1–6`               | Envanter hotkey item kullan                         |
| `Q`, `W`, `E`, `R`          | Hero skill kullan                                   |
| `I`                          | Envanter panelini aç / kapat                        |
| `TAB`                        | Kule seçim panelini aç / kapat                      |
| `P` veya `ESCAPE`            | Duraklat / Devam et                                 |
| `F`                          | Oyun hızı değiştir (1× → 2× → 4× → 1×)            |
| `G`                          | Grid çizgilerini göster / gizle                     |
| `H`                          | Kamerayı Hero'ya kilitle / serbest bırak            |
| `SPACE`                      | Diyalogda ileri / Dalga arasında sonraki dalgayı başlat |
| `E`                          | Yakındaki interactable ile etkileşim                |
| Scroll Wheel                 | Kamera zoom in/out                                  |
| Orta fare sürükle            | Kamera pan                                          |

---

## 15. VERİ AKIŞI (DATA FLOW) DİYAGRAMI

```
                        ┌──────────────────┐
                        │    main.c        │
                        │  (Game Loop)     │
                        └───────┬──────────┘
                                │
               ┌────────────────┼────────────────┐
               │                │                │
         ┌─────▼─────┐   ┌─────▼─────┐   ┌─────▼──────┐
         │ HandleInput│   │  Update   │   │   Draw     │
         │  (ui.c)   │   │ (game.c)  │   │  (game.c)  │
         └─────┬─────┘   └─────┬─────┘   └─────┬──────┘
               │                │                │
    ┌──────────┼────────────────┼────────────────┤
    │          │                │                │
    ▼          ▼                ▼                ▼
┌───────┐ ┌───────┐ ┌─────────────────┐ ┌──────────┐
│ Hero  │ │Towers │ │    Enemies      │ │ Particle │
│(hero.c│ │(tower │ │   (enemy.c)     │ │(particle │
│  .c)  │ │  .c)  │ │ + Boss AI       │ │   .c)    │
└───┬───┘ └───┬───┘ └───────┬─────────┘ └──────────┘
    │         │             │
    │    ┌────▼────┐   ┌────▼────┐
    │    │Projectile│   │  Wave  │
    │    │(projectile│  │(wave.c)│
    │    │   .c)    │   └────────┘
    │    └─────────┘
    │
    ├──► Inventory (inventory.c) ◄── LootDrop (enemy.c)
    │
    ├──► Skills (hero.c) ──► Particle/Projectile spawn
    │
    └──► Interactable (map.c) ──► Inventory / Stats buff

Haberleşme: Tüm alt sistemler Game struct'ına pointer alır (Game *game).
Hiçbir alt sistem diğerini doğrudan çağırmaz — hepsi Game üzerinden haberleşir.
Örnek: enemy.c ölen düşmanın XP'sini hero'ya eklemez, bunun yerine
       game->pendingXP += reward şeklinde Game'e yazar, game.c bunu hero'ya aktarır.
```

---

## 16. ANA DÖNGÜ MİMARİSİ

```c
int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tower Defense RPG — C & Raylib");
    SetTargetFPS(60);
    InitAudioDevice();

    Game game = {0};
    InitGame(&game);

    while (!WindowShouldClose() || game.state == STATE_PAUSED) {
        // ESC'yi kendimiz yönetiyoruz — WindowShouldClose'u override et
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (game.state == STATE_PLAYING)
                game.state = STATE_PAUSED;
            else if (game.state == STATE_PAUSED)
                game.state = STATE_PLAYING;
        }

        float rawDt = GetFrameTime();
        float dt = rawDt * game.gameSpeed;  // Hızlandırma çarpanı

        // ─── INPUT ───
        HandleGlobalInput(&game);           // Hız, grid toggle, kamera zoom

        // ─── UPDATE ───
        switch (game.state) {
            case STATE_MENU:
                UpdateMenu(&game);
                break;

            case STATE_PLAYING:
                UpdateCamera(&game, rawDt);                 // Kamera her zaman rawDt
                HandlePlayingInput(&game);                  // Kule koyma, hero komutu
                UpdateHero(&game, dt);                      // Hero hareket, saldırı, skill
                UpdateEnemies(&game, dt);                   // Düşman hareketi
                UpdateBoss(&game, dt);                      // Boss AI (varsa)
                UpdateTowers(&game, dt);                    // Hedefleme, ateş
                UpdateProjectiles(&game, dt);               // Mermi hareket, isabet
                UpdateParticles(&game, dt);                 // Efektler
                UpdateFloatingTexts(&game, dt);             // Hasar sayıları
                UpdateLootDrops(&game, dt);                 // Yerdeki itemler
                UpdateInteractables(&game, dt);             // Harita objeleri
                UpdateWaves(&game, dt);                     // Dalga spawn
                UpdateScreenShake(&game, rawDt);            // Sarsıntı (rawDt!)
                ProcessPendingEvents(&game);                // XP, ödül, diyalog tetikleme
                CheckGameConditions(&game);                 // Can 0? Son dalga bitti?
                break;

            case STATE_PAUSED:
                UpdatePauseMenu(&game);
                break;

            case STATE_DIALOGUE:
                UpdateDialogue(&game.dialogue, rawDt);      // Diyalog rawDt ile (hız etkilemesin)
                break;

            case STATE_GAMEOVER:
            case STATE_VICTORY:
                UpdateEndScreen(&game);
                break;
        }

        // ─── DRAW ───
        BeginDrawing();
        ClearBackground(COLOR_BG);

        switch (game.state) {
            case STATE_MENU:
                DrawMenu(&game);
                break;

            case STATE_PLAYING:
            case STATE_PAUSED:
            case STATE_DIALOGUE:
                BeginMode2D(game.camera.raylibCam);         // Kamera transform
                    DrawMap(&game);                          // Grid, yol, tile animasyonları
                    DrawInteractables(&game);                // Harita objeleri
                    DrawTowerSynergies(&game);               // Sinerji çizgileri
                    DrawTowers(&game);                       // Kuleler
                    DrawEnemies(&game);                      // Düşmanlar + HP bar
                    DrawBoss(&game);                         // Boss (varsa)
                    DrawHero(&game);                         // Kahraman
                    DrawProjectiles(&game);                  // Mermiler + trail
                    DrawLootDrops(&game);                    // Yerdeki itemler
                    DrawParticles(&game);                    // Efektler
                    DrawFloatingTexts(&game);                // Hasar sayıları
                    DrawPlacementPreview(&game);             // Kule yerleştirme önizlemesi
                EndMode2D();

                DrawHUD(&game);                              // Üst panel, hero panel, kule seçim

                if (game.state == STATE_PAUSED)
                    DrawPauseOverlay(&game);
                if (game.state == STATE_DIALOGUE)
                    DrawDialogue(&game.dialogue);
                if (game.inventory.isOpen)
                    DrawInventory(&game);
                break;

            case STATE_GAMEOVER:
                DrawGameOverScreen(&game);
                break;

            case STATE_VICTORY:
                DrawVictoryScreen(&game);
                break;
        }

        EndDrawing();
    }

    UnloadGame(&game);      // Ses dosyalarını ve kaynakları serbest bırak
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
```

---

## 17. BELLEK YÖNETİMİ KURALLARI

```
TEMEL KURAL: Statik diziler kullan, dinamik bellek yönetimini minimize et.

Statik (stack / global):
  - enemies[MAX_ENEMIES]         → Her zaman sabit boyut, active flag ile yönet
  - towers[MAX_TOWERS]           → Aynı yaklaşım
  - projectiles[MAX_PROJECTILES] → Aynı
  - particles[MAX_PARTICLES]     → Ring buffer veya active flag
  - floatingTexts[MAX_FLOATING_TEXTS] → Active flag

Dinamik (malloc gerekirse):
  - Inventory item stack'leri → HAYIR, sabit INVENTORY_SLOTS yeterli
  - Dialogue satırları → Sabit DIALOGUE_MAX_LINES dizisi, malloc gereksiz
  - Wave tanımları → Sabit MAX_WAVES dizisi

malloc KULLANILACAK TEK YER (opsiyonel):
  - Çok büyük harita dosyası yükleme (ileride)
  - SaveGame/LoadGame serileştirme buffer'ı

Havuz (Pool) Deseni:
  Her spawn fonksiyonu, dizide "active == false" olan ilk elemanı bulur.
  Bulamazsa → spawn'ı sessizce atla (crash etme!).
  int FindFreeSlot(void *array, int maxCount, size_t elemSize, size_t activeOffset);
```

---

## 18. ÇIKTI BEKLENTİSİ

1. Önce **Veri Akışı (Data Flow)** şemasını açıkla — hangi sistem hangi sisteme nasıl veri aktarıyor.
2. Tüm `struct` tanımlarını ve `enum`'ları içeren `types.h` dosyasını sağla.
3. Tüm `#define` sabitlerini içeren `config.h` dosyasını sağla.
4. Her alt sistem için fonksiyon prototiplerini içeren header dosyalarını sağla.
5. `main.c` ve tüm `.c` dosyalarını sağla — modüler yapıda ama **her dosya bağımsız derlenebilir**.
6. `Makefile` sağla.
7. Kod derlendiğinde **çalışan, oynanabilir** bir oyun olmalı:
   - Menü ekranında sınıf seçimi (Warrior / Mage / Archer)
   - Hero WASD veya sağ tık ile hareket eder
   - Kuleler yerleştirilir, düşmanlar gelir, mermiler uçar
   - Hero otomatik saldırır, Q/W/E/R ile skill kullanır
   - Boss dalgalarında çok fazlı boss savaşı yaşanır
   - Envanter ve item sistemi çalışır
   - Diyalog kutuları uygun zamanlarda belirir
   - Tüm görsel efektler (parçacık, trail, shake, floating text) çalışır
8. Her fonksiyonun başında Türkçe yorum satırı olmalı.
9. Karmaşık algoritmalar (boss AI, skill hesaplama, sinerji tespiti) adım adım açıklanmalı.

---

## 19. BONUS (İSTENİR AMA ZORUNLU DEĞİL)

- **Minimap**: Sağ üst köşede küçük harita görünümü (düşman noktaları, kule noktaları, hero).
- **Kill Combo**: 2 saniye içinde 3+ kill → ekstra altın + "TRIPLE KILL!" floating text.
- **Dalga Arası Mağaza**: Dalgalar arasında item satın alma paneli.
- **Otomatik Kayıt**: Her dalga sonunda oyun durumunu basit bir dosyaya yaz/oku.
- **Tooltip Sistemi**: Fare herhangi bir UI elemanının üzerine gelince açıklama baloncuğu.
- **Başarım Sistemi**: "İlk Boss'u Yen", "100 Düşman Öldür", "Hiç Can Kaybetme" gibi basit başarımlar.

### PROJECT EXPANSION: TD-RPG-STRATEGY HYBRID (C & Raylib)

**Context:** Elimizde çalışan bir Tower Defense motoru var. Şimdi bu motoru Age of Empires 3, Dota ve Knight Online mekanikleriyle hibrit bir yapıya taşıyoruz.

**1. Home City & Reinforcement System (AoE 3 Mantığı):**
- **Home City Struct:** Oyuncunun bir "Medeniyet Seviyesi" (CivLevel) ve "Sevkiyat Puanı" (ShipmentPoints) olsun.
- **Card/Perk System:** Oyun sırasında kazanılan puanlarla "Ana Kent"ten takviye (Archers, Wood, Tech Upgrade) talep edilebilecek bir kart/menü sistemi kurgula.
- **Civilization Progress:** Halkı korudukça artan bir "Refah Puanı" (Prosperity) ekle.

**2. Isometric View & Siege Mechanics:**
- **Isometric Projection:** Koordinat sistemini izometrik bakışa (45° rotation, 0.5 y-scale) dönüştürecek matris yapısını kur.
- **Siege Map Scenario:** Yay şeklinde devasa bir sur yapısı (Wall Segment array) tanımlanmalı.
- **Unit Stations:** Surların üzerine "Archer" ve "Cannon" yerleştirilebilirken; kapı arkasına "Cavalry" (Süvari) gibi hücum birimleri konuşlandırılabilecek özel grid slotları oluştur.
- **Army Batallions:** Düşmanları tek tek değil, "Kıta" (Horde/Battalion) halinde hareket eden gruplar olarak yönet.

**3. Base Building (Waves Interval):**
- Dalgalar arasında (State: PREP_PHASE), oyuncunun haritaya "Barracks", "Market" veya "Barricade" inşa edebileceği bir bina sistemi ekle.

**4. Dungeon Mode (RPG/Farming):**
- **State Change:** `STATE_DUNGEON` modunu ekle. Burada Tower Defense grid'i yerine, Hero'nun serbestçe hareket edip Knight Online/Dota tarzı canavarlardan (mobs) loot ve XP topladığı kapalı alan haritaları kurgula.
- **Loot Table:** Dungeon'da düşen eşyaların (Pot, Rune, Gear) Hero'nun `Inventory` struct'ına aktarılmasını sağla.

**5. Technical Architecture (C/Raylib):**
- Bu modülleri `homecity.h`, `dungeon.h` ve `siege.h` olarak ayır.
- Isometric Z-sorting (Y eksenine göre sıralı çizim) algoritmasını basitçe göster.
- Mouse input'u izometrik koordinatlara çeviren `GetIsometricMousePos()` fonksiyonunu sağla.

**Çıktı Beklentisi:**
Lütfen önce bu karmaşık sistemin (Home City -> TD Map -> Dungeon) veri akış diyagramını açıkla. Ardından, izometrik görünümün temel matematiksel altyapısını ve Home City sevkiyat sistemini içeren C kodlarını sağla.

---

## GENIŞLETME: MOBA KONTROL ŞEMASI VE KAHRAMAN DİNAMİKLERİ

### 1. Kontrol Şeması ve Kahraman Dinamikleri

Oyun, saf bir stratejiden ziyade bir **"Yönetici Kahraman"** hissi üzerine kurulu.

**MOBA Kontrolleri (Dungeon Modu):**
- **Sağ Tık (RMB):** Karakterin hareketi sağ tıkla yapılır — tıklanan noktaya Pathfinding ile yürür.
- **Sol Tık (LMB):** Hedef seçimi / saldırı emri.
- **Q / W / E / R:** Hero-centric yetenekler (aktif skill slotları). Bu yetenekler:
  - Hasarın yanı sıra binalara geçici **buff** verebilir (örn. Barracks üretim hızı +%20).
  - Savunma hatlarına taktiksel avantajlar sağlar: sis perdesi, alan yavaşlatma, kalkan teli vb.
- **MEVCUT KONTROLLERE DEĞIŞIKLIK:** Dungeon modunda `WASD / Ok tuşları` yerine artık **sağ tık + pathfinding** kullanılır; `SPACE / Sol Tık` saldırı yerine `Q-W-E-R` skill sistemi devreye girer.

**Teknik Gereksinimler — MOBA Kontrol Sistemi:**
```c
// Hero pathfinding için eklenmesi gereken struct alanları
typedef struct {
    Vector2 position;
    Vector2 targetPos;       // Sağ tıklanan hedef nokta
    bool isMoving;           // Pathfinding aktif mi
    float moveSpeed;
    // Skill sistemi
    float skillCooldown[4];  // Q=0, W=1, E=2, R=3
    float skillMaxCD[4];
    int   skillLevel[4];
} HeroMOBA;

// Skill tetikleme: sağ tık hedef set, Q/W/E/R skill fire
// RIGHT_MOUSE_BUTTON → hero.targetPos = GetMouseWorldPos()
// KEY_Q/W/E/R       → FireSkill(hero, skillIndex, mouseTarget)
```

**Skill Tasarım İlkeleri:**
| Tuş | Örnek Yetenek          | Etki Türü                            |
|-----|------------------------|--------------------------------------|
| Q   | Kılıç Darbesi          | Yakın çevreye hasar (cone AOE)       |
| W   | Komuta Çığlığı         | Yakın Barracks'a +%20 üretim buff    |
| E   | Sis Perdesi            | Belirtilen alana 5s görüş engeli     |
| R   | Ulti: Kuşatma Sinyali  | Tüm kulelere 10s +%30 hasar bonusu   |

---

### 2. Devasa Harita ve Coğrafi Yerleşim

Harita, soldan sağa doğru bir **"Yıpratma Savaşı"** koridoru gibi tasarlanmıştır.

| Bölge                    | Açıklama                                                                 |
|--------------------------|--------------------------------------------------------------------------|
| **Batı (Vahşi Topraklar)** | Düşman dalgalarının doğduğu karanlık bölgeler                          |
| **Doğu (Medeniyetin Sınırı)** | Şehrin kurulduğu güvenli liman; dağlar/nehirlerle korunan             |
| **Ara Bölge (No Man's Land)** | Kule inşası, kaynak toplama ve ilk savunma hatları için stratejik alan|

**Fog of War (Savaş Sisi):**
- Oyuncu şehirden uzaklaştıkça risk artar, keşfedilecek kaynaklar büyür.
- Hero'nun görüş yarıçapı dışındaki alanlar sis ile kaplanır.
- Önceden keşfedilmiş alan gri tonla (last-seen) gösterilir, aktif görüş alanı tam renklidir.

```c
// Fog of War temel yapısı
typedef enum { FOG_HIDDEN = 0, FOG_SEEN = 1, FOG_VISIBLE = 2 } FogState;
FogState fogMap[MAP_ROWS][MAP_COLS]; // Her hücre için sis durumu
float heroVisionRadius = 200.0f;    // piksel cinsinden görüş yarıçapı
```

---

### 3. Kendi Kendini İdame Ettiren Şehir Ekonomisi

Şehir sadece bir kale değil, **yaşayan bir organizmadır.**

| Bina                      | İşlev                                                                      |
|---------------------------|----------------------------------------------------------------------------|
| **Town Center**           | Vergi toplama hızı ve nüfus kapasitesini belirler; şehrin kalbi           |
| **Trade Center**          | Çevre bölgelerle otomatik kervan ticareti → pasif altın akışı             |
| **Taverna**               | Hero morali, paralı asker kiralama, gezgin bilginlerden geçici bonuslar   |
| **Okçu Akademisi**        | Ünite üretimi + pasif araştırma: kule menzili artışı                      |
| **Mage Akademisi**        | Ünite üretimi + pasif araştırma: büyü hasarı / kule splash radius artışı  |

**Akademi Araştırma Ağacı (Research Tree) — Teknik:**
```c
typedef struct {
    int tier;           // 1-3 arası yükseltme seviyesi
    float bonusRange;   // Tüm kulelere eklenen menzil bonusu (piksel)
    float bonusDamage;  // Tüm kulelere eklenen hasar çarpanı
    bool unlocked;
} AcademyResearch;
```

---

### 4. Prosperity (Refah) Puanı ve Otonom Savunma

Oyunun en özgün mekaniği: her bölümün bir **"Kazanma Eşiği"** (Refah Puanı) vardır.

**Refah Puanı Nasıl Artar?**
- Bina inşası, başarılı ticaret yolları
- Öldürülen boss'lar
- Halkın güvenliği (can kaybı olmayan dalgalar)

**Otonom Evre (Refah Eşiği Aşıldığında):**
- Binalar kendi kendine onarılır.
- Barracks otomatik olarak bölüğünü çıkarıp cephe hattına gönderir.
- Kuleler oyuncu komutuna gerek duymadan en optimize hedeflere saldırır.

**Kritik Eşik ve Boss Wave:**
- Savunma hattı çökerse veya ekonomi sabote edilirse Refah Puanı düşer.
- Puan eşiğin altına indiğinde → **"Liderlik Krizi"** sinyali → Final Boss Wave tetiklenir.
- Sonuç: mutlak zafer **ya da** şehrin düşüşü.

```c
// Prosperity sistemi için sabitler
#define PROSPERITY_AUTONOMY_THRESHOLD  500   // Otonom evreye geçiş eşiği
#define PROSPERITY_CRISIS_THRESHOLD    100   // Boss Wave tetikleyici eşik
#define PROSPERITY_PER_KILL            5     // Her düşman öldürmede kazanılan puan
#define PROSPERITY_PER_WAVE_CLEAR      50    // Dalga temizleme bonusu

typedef struct {
    int score;
    bool autonomousMode;  // Eşik aşıldığında true
    bool crisisMode;      // Eşik altına düştüğünde true, boss wave tetikler
} ProsperitySystem;
```

---

### 5. Süreklilik ve Kalıcı Dünya (Persistence)

Bölüm başarıyla geçildikten sonra oyuncu burayı terk etmek **zorunda değildir.**

**Geri Dönüş (Retake):**
- Oyuncu eski bir bölüme girdiğinde kurduğu medeniyeti bıraktığı gibi bulur.
- Gelişmiş ekonomisiyle oradan kaynak aktarabilir.
- **"Yık-Yeniden Yap" (Creative Destruction):** Şehri modernize etmek için binaları yıkıp daha verimli düzende yeniden inşa edebilir.

**Zindan (Dungeon) Erişim Koşulu:**
- Dungeonlar yalnızca Refah Puanı eşiği geçildikten **ve** bölüm "temizlendikten" sonra açılır.
- Şehrin güvenliği sağlandığı için kahraman tüm dikkatini bu karanlık mahzenlere verebilir.
- Dungeonlar: yüksek riskli / yüksek ödüllü **end-game** içerikleridir.

---

### 6. Güncellenen Kontrol Referansı (Dungeon Modu)

| Girdi            | Eylem                                               |
|------------------|-----------------------------------------------------|
| **Sağ Tık**      | Hero'yu tıklanan noktaya yönlendir (Pathfinding)    |
| **Sol Tık**      | Hedef seç / yakın saldırı                           |
| **Q**            | 1. Yetenek (Kılıç Darbesi / Koni AOE)               |
| **W**            | 2. Yetenek (Komuta Çığlığı / Barracks buff)         |
| **E**            | 3. Yetenek (Sis Perdesi / görüş engeli)             |
| **R**            | 4. Yetenek / Ulti (Kuşatma Sinyali / kule buff)    |
| **ESC**          | Dungeon'dan çık (altın bonus aktarımı)              |

---

## SON BÖLÜM TASARIMI: MERKEZ KALE KUŞATMASI

### 1. Harita Geometrisi ve Görsel Dil

- **Şehir konumu:** Haritanın tam geometrik merkezinde. Tüm yönlerden (kuzey, güney, doğu, batı, köşegen) eş zamanlı düşman saldırısı gelir.
- **Zemin görünümü:** Şehir surlarının dışı bom boş buğday tarlaları. Parlak sarı-yeşil açık alan — hiçbir doğal engel yok. Düşman hareketleri uzaktan net okunabilir.
- **Renk paleti:** Yazlık tarla altını (`#D4A017`), kuru toprak (`#8B6914`), şehir taşları koyu gri. Savaş kızılı (kan, ateş) kontrast olarak.

```c
// Son bölüm tile tipleri
typedef enum {
    TILE_WHEAT_FIELD = 10,  // Buğday tarlası — geçilebilir, engelsiz
    TILE_CITY_WALL   = 11,  // Sur — geçilemez
    TILE_CITY_INNER  = 12,  // Şehir içi — güvenli bölge
    TILE_ROAD        = 13,  // Destek yolu — kesilince gri olur
    TILE_ELEVATED    = 14,  // Kürekle yükseltilmiş zemin — kule yapılabilir
} FinalLevelTile;
```

### 2. Kürek / Elevate Mekaniği (Sadece Son Bölüm)

Normal TD haritasında kuleler doğrudan yerleştirilebilir. Son bölümde **düz zemine (buğday tarlası) direkt kule kurulamaz.** Önce zeminin yükseltilmesi gerekir.

**Akış:**
1. Oyuncu kürek moduna girer (`E` tuşu veya UI paneli).
2. Hedef hücreye sol tıklanır → animasyon + altın maliyeti (`ELEVATE_COST 30`).
3. Hücre `TILE_ELEVATED`'e geçer, üstüne artık kule kurulabilir.
4. Yükseltme iptal edilemez (kalıcı harita değişimi).

```c
#define ELEVATE_COST       30    // Altın
#define ELEVATE_TIME       1.5f  // Saniye (animasyon süresi)
#define MAX_ELEVATED_CELLS 40    // Son bölüm için statik limit

typedef struct {
    int   gridX, gridY;
    float progress;  // 0→1 (kazıma animasyonu)
    bool  done;
} ElevateJob;

ElevateJob elevateQueue[MAX_ELEVATED_CELLS]; // Aktif kazıma kuyruğu
```

**Görsel:** Hücre üzerine küçük toprak yığını çizilir, yükseltme tamamlanınca açık kahverengi (`TILE_ELEVATED`) görünür.

### 3. Krallık Minimap — Metro Haritası Stili

Ekranın **üst-orta** kısmında sabit 320×80 piksellik bir şerit harita bulunur.

**Tasarım dili (metro haritası):**
- Her bölüm/şehir → renkli daire (node)
- Şehirler arası bağlantılar → düz veya L-şekli çizgiler (metro line stili)
- Aktif bölüm → parlak beyaz kenarlıklı, diğerleri mat
- Kesilmiş yol → kırmızı X işareti, çizgi gri olur
- Dimension Gate → kırmızı/mor nabız atan ikon

```
 [Köy A]──[Köy B]──[Kasaba C]──[Kale D]──[ANA KENT]
    ○────────○────────◇──────────◇────────●
             │               Dimension Gate ↗
          [Liman]              ▲
                            [Köy E]
```

**Renk kodlaması:**
| Durum                | Renk             |
|----------------------|------------------|
| Güvenli bağlantı     | `GREEN`          |
| Tehdit altında       | `YELLOW`         |
| Kesilmiş yol         | `RED` (+ gri çizgi) |
| Dimension Gate aktif | `PURPLE` nabız  |
| Ana kent             | `GOLD` + büyük   |

```c
#define KINGDOM_MAX_CITIES  8
#define KINGDOM_MAX_ROADS   10

typedef struct {
    char  name[20];
    float mapX, mapY;   // Minimap üzerindeki koordinat (0.0–1.0)
    int   civLevel;
    bool  connected;    // Hâlâ ana kente bağlı mı
    bool  razed;        // Düşman tarafından tahrip edildi mi
    int   soldierCount; // Destek çağırılabilecek asker sayısı
} KingdomCity;

typedef struct {
    int  cityA, cityB;  // Şehir indeksleri
    bool blocked;       // Yaratıklar tarafından kesildi mi
    float blockProgress;// 0→1 (kesilme animasyonu)
} KingdomRoad;

typedef struct {
    KingdomCity cities[KINGDOM_MAX_CITIES];
    KingdomRoad roads[KINGDOM_MAX_ROADS];
    int         cityCount;
    int         roadCount;
    int         currentWave; // Hangi dalgadayız (yol kesme hesabı için)
} KingdomMap;
```

### 4. Destek Çağırma Skilli — R Tuşu

**Şehirlerden Asker Çağırma (Dungeon + TD haritasında aktif):**

Oyuncu önceki bölümlerde kurduğu şehirlerin `soldierCount` havuzundan destek çağırabilir.

**Mekanik:**
- `R` tuşuna basınca: Bağlı şehirlerin toplam asker kapasitesi hesaplanır.
- Çağrı animasyonu: Minyatür askerler haritanın kenarından içeriye koşar.
- Askerler hero etrafında konumlanır, otomatik savaşır, 30 saniye sonra dönerleri.
- Cooldown: 120 saniye (bölüm başına 3–4 kez kullanılabilir).
- **Kısıt:** Yolu kesilmiş şehirden asker gelmez → minimapta animasyon + ses efekti.

```c
#define ALLY_CALL_COOLDOWN   120.0f  // saniye
#define ALLY_MAX_COUNT         5     // Aynı anda sahada max müttefik
#define ALLY_DURATION         30.0f  // sahada kalma süresi

// Hangi şehirlerden kaç asker gelebilir
int ComputeAvailableSoldiers(KingdomMap *km);  // Bağlı şehirlerin toplamı
void SpawnAllyWave(AllyUnit allies[], KingdomMap *km, Vector2 heroPos);
```

**Görsel geri bildirim:**
- Çağrı başarılı → ekran kenarından mavi ışık halkası gelir.
- Yol kesilmiş → kırmızı "YOL KESİLDİ" floating text + minimap'te kırmızı yanıp söner.
- Tüm yollar kesildi → R devre dışı, buton gridir.

### 5. Yol Kesme Sistemi — Wave Eş Zamanlı

Dalgalar ilerledikçe Dimension Gate'ten çıkan yaratıklar şehirler arası yolları keser.

**Kesme Takvimi (örnek 10 dalgalık son bölüm):**
| Dalga | Olay                                          |
|-------|-----------------------------------------------|
| 1–2   | Tüm yollar açık, R skilli tam kapasite        |
| 3     | [Köy A] yolu tehdit altına girer (YELLOW)     |
| 4     | [Köy A] bağlantısı kesilir — asker gelmez     |
| 5     | Dimension Gate genişler, 2. yol tehdit altında|
| 6     | [Liman] bağlantısı kesilir                    |
| 7–8   | Sadece 2–3 şehir bağlı kalır                 |
| 9     | Son bağlı şehir de tehdit altında             |
| 10    | **BOSS WAVE** — tüm yollar kesilmiş, tek başına|

```c
void UpdateKingdomRoads(KingdomMap *km, int currentWave) {
    // Her wave geçişinde road blocking mantığı
    // Örn: wave >= 4 → roads[0].blocked = true
    // Animasyonlu geçiş: blockProgress 0→1 artar
}
```

### 6. Dimension Gate (Cehennem Kapısı)

Haritanın ana kente yakın (ancak surların dışında) konumlanan **boyutsal geçit.**

**Özellikler:**
- Görsel: Mor-kırmızı dönen portal, üzerinde kayan rünler. Her wave ile büyür.
- İlk dalgada küçük ve pasif. Son wave'de neredeyse sur boyutu.
- Gate'ten hem normal düşmanlar hem **Road Blocker** yaratıklar çıkar.
- Road Blocker'lar: Yavaş hareket eder, yollara saparak mini "barikat noktası" kurarlar. Öldürüldüğünde yol yeniden açılır (kısa süreliğine).
- Gate yok edilemez (lore: boyutlar arası bağlantı sabit).

```c
typedef struct {
    Vector2 position;
    float   size;        // Dalgaya göre büyür (wave * 8.0f + 40.0f)
    float   pulseTimer;  // Nabız animasyonu
    int     wave;        // Mevcut dalga (görsel büyüklük için)
    Color   innerColor;  // (200, 0, 255, 200) → (255, 0, 0, 255)
    Color   outerColor;
} DimensionGate;

void DrawDimensionGate(DimensionGate *gate) {
    // Dönen dış halka
    // İç portal (lerp renk)
    // Nabız efekti: sin(pulseTimer * 3.0f) * 10.0f boyut salınımı
}
```

### 7. Boss Wave — Final Düşman

Son dalgada Gate'ten çıkan tek dev boss.

| Özellik       | Değer                                              |
|---------------|----------------------------------------------------|
| HP            | Mevcut Boss HP × 5                                 |
| Hız           | Yavaş (surları doğrudan hedefler)                  |
| Özel saldırı  | Her 10 saniyede bir AoE şok dalgası                |
| Özel davranış | Yolu kesilmiş bölgelerden güç devşirir (+%10 HP regen) |
| Ölüm ödülü    | Massive gold + Krallık Zaferi ekranı               |

```c
#define BOSS_FINAL_HP_MULT     5.0f
#define BOSS_FINAL_SHOCK_CD   10.0f
#define BOSS_FINAL_SHOCK_RADIUS 120.0f
#define BOSS_FINAL_SHOCK_DMG   25.0f
```

### 8. Teknik Özet — Son Bölüm Eklemeleri

Yeni eklenmesi gereken modüller ve bağımlılıklar:

```
main.c
  ├── kingdom.h   → KingdomMap, KingdomCity, KingdomRoad
  ├── finalmap.h  → ElevateJob, DimensionGate, FinalLevelTile
  └── dungeon.h   → AllyUnit (R skilli çağrılanlar)

Yeni fonksiyonlar:
  DrawKingdomMinimap(KingdomMap *km, int screenW)
  UpdateKingdomRoads(KingdomMap *km, int wave)
  SpawnAllyWave(AllyUnit[], KingdomMap*, Vector2 heroPos)
  DrawDimensionGate(DimensionGate *gate)
  UpdateElevateJobs(ElevateJob[], int *gold)
  DrawElevatedCells(ElevateJob[])
```