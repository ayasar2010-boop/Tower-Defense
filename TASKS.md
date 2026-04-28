# TASKS — Tower Defense Geliştirme Planı

Tüm kod `src/main.c` tek dosyasında toplanır.  
Her görev tamamlandığında `[x]` ile işaretlenir.

---

## AŞAMA 1 — İskelet ve Temel Yapı
- [x] **T01** — Dizin yapısını oluştur (`src/`, `assets/`)
- [x] **T02** — `Makefile` yaz (Linux + Windows hedefleri)
- [x] **T03** — `src/main.c` iskeleti: `#include`, `#define` sabitleri, enum'lar
- [x] **T04** — Tüm struct tanımları: `Enemy`, `Tower`, `Projectile`, `Particle`, `Wave`, `Game`
- [x] **T05** — `main()` game loop iskelet: pencere, 60 FPS, state switch, `BeginDrawing/EndDrawing`

## AŞAMA 2 — Harita ve Navigasyon
- [x] **T06** — `InitMap()`: 20×12 grid, hardcoded yol + buildable hücreler
- [x] **T07** — `InitWaypoints()`: Piksel waypoint dizisi
- [x] **T08** — `DrawMap()`: Hücre tipine göre renkli grid çizimi
- [x] **T09** — Grid çizgisi toggle (`showGrid`)
- [x] **T10** — `GridToWorld()` / `WorldToGrid()` dönüşüm fonksiyonları

## AŞAMA 3 — Düşman Sistemi
- [x] **T11** — `SpawnEnemy()`: Tip tablosuna göre HP/hız/renk/yarıçap ata
- [x] **T12** — `UpdateEnemies()`: Waypoint takibi, `slowTimer`, son waypoint → `lives--`
- [x] **T13** — `DrawEnemies()`: Tip şekli (daire/üçgen/kare) + HP bar

## AŞAMA 4 — Kule Sistemi
- [x] **T14** — `CanPlaceTower()` doğrulama (sınır + buildable + altın)
- [x] **T15** — Kule yerleştirme: Sol tık → hücreyi `CELL_TOWER` yap, altın düş
- [x] **T16** — `FindNearestEnemy()`: Menzil içindeki en yakın aktif düşman
- [x] **T17** — `UpdateTowers()`: `fireCooldown`, hedef seçimi, `CreateProjectile()`
- [x] **T18** — `DrawTowers()`: Kare gövde + namlu çizgisi (rotation)
- [x] **T19** — Kule yükseltme: Sağ tık → level++, hasar/menzil artışı

## AŞAMA 5 — Mermi ve Parçacık Sistemi
- [x] **T20** — `CreateProjectile()`: Homing vektör, splash bilgisi
- [x] **T21** — `UpdateProjectiles()`: Hareket, isabet tespiti, splash hasar
- [x] **T22** — `DrawProjectiles()`: Kule tipine göre renk/şekil
- [x] **T23** — `SpawnParticles()`: Ölüm / isabet / splash efektleri
- [x] **T24** — `UpdateParticles()`: Fade + shrink, `lifetime <= 0` deaktive
- [x] **T25** — `DrawParticles()`: Alpha-blended daire/kare

## AŞAMA 6 — Dalga Sistemi
- [x] **T26** — `InitWaves()`: 10 dalganın tanımı (tip, sayı, aralık, ön-gecikme)
- [x] **T27** — `UpdateWaves()`: `spawnTimer`, `SpawnEnemy()` tetikleme
- [x] **T28** — Dalga tamamlanma tespiti: Tüm düşmanlar öldü/geçti → `STATE_WAVE_CLEAR`
- [x] **T29** — Son dalga → `STATE_VICTORY`

## AŞAMA 7 — HUD ve Girdi
- [x] **T30** — `DrawHUD()`: Üst panel (can/altın/skor/dalga), kule seçim butonları
- [x] **T31** — Yerleştirme önizlemesi: Fare grid'deyken yeşil/kırmızı yarı saydam kare + menzil dairesi
- [x] **T32** — `HandleInput()`: 1/2/3 kule seçimi, sol tık yerleştir, sağ tık yükselt, P/ESC/F/G/SPACE/R

## AŞAMA 8 — Durum Ekranları
- [x] **T33** — `DrawMenu()` + `UpdateMenu()`: Başlık + "Başla" butonu
- [x] **T34** — `DrawPauseOverlay()` + `UpdatePause()`: Karartma + Devam/Çık
- [x] **T35** — `DrawGameOver()`: GAME OVER + skor + Yeniden Başla
- [x] **T36** — `DrawVictory()`: TEBRİKLER + final skor + Yeniden Başla
- [x] **T37** — `InitGame()` / `RestartGame()`: Sıfırlama fonksiyonu

## AŞAMA 9 — Derleme ve Test
- [x] **T38** — `Makefile` doğrula, `make` ile temiz derleme
- [x] **T39** — Oynanabilirlik testi: Tüm dalgalar, kule yerleştir/yükselt, game over/victory (splash ölüm hatası düzeltildi)
- [x] **T40** — Bellek / sınır kontrolü: Dizi overflow yok, grid sınır aşımı yok

## BONUS (İsteğe Bağlı)
- [x] **B01** — Kule satma (sağ tık menüsü, %50 geri)
- [x] **B02** — Yol yön okları
- [x] **B03** — Dalga ilerleme çubuğu (mini progress bar)
- [x] **B04** — Kule ateş flash animasyonu

---

## Durum Özeti
| Aşama | Toplam | Tamamlanan |
|-------|--------|------------|
| 1 — İskelet      | 5  | 5 |
| 2 — Harita       | 5  | 5 |
| 3 — Düşman       | 3  | 3 |
| 4 — Kule         | 6  | 6 |
| 5 — Mermi/Partik | 6  | 6 |
| 6 — Dalga        | 4  | 4 |
| 7 — HUD/Girdi    | 3  | 3 |
| 8 — Ekranlar     | 5  | 5 |
| 9 — Test         | 3  | 3 |
| **Toplam**       | **40** | **40** |

## Durum Özeti (Aşama 10)

| Aşama                | Toplam | Tamamlanan |
|----------------------|--------|------------|
| 10 — VS Code Ortamı  | 6      | 6          |

## AŞAMA 10 — VS CODE GAME STUDIO ORTAMI

- [x] **T46** — `.vscode/extensions.json`: C/C++ Pack, CMake/Makefile Tools, Better C++ Syntax, Shader dil desteği, Asset önizleyici, Hex Editor, GitLens, TODO-Tree önerileri
- [x] **T47** — `.vscode/settings.json`: Clang-Format kuralları, build artifact exclude listesi, Raylib IntelliSense yolları (`C:/msys64/mingw64/include`), GLSL dosya ilişkilendirme, Git Bash terminali
- [x] **T48** — `.vscode/c_cpp_properties.json`: MSYS2 MinGW64 derleyici yolu, Raylib include yolu, C99 standardı, `PLATFORM_DESKTOP` define
- [x] **T49** — `.vscode/tasks.json`: `make` (default build, Ctrl+Shift+B), `make clean`, `make run`, `Rebuild (clean+build)` görevleri + gcc problem matcher
- [x] **T50** — `.vscode/launch.json`: F5 → GDB debug (`game.exe`), `preLaunchTask: "Build: make"`, pretty-printing, debug'suz çalıştır konfigürasyonu
- [x] **T51** — `CLAUDE.md` güncellendi: Çok dosyalı mimari (`homecity`, `siege`, `dungeon`), VS Code kısayolları, modül bağımlılıkları, kod kuralları eklendi

---

## AŞAMA 11 — TD-RPG-STRATEGY HYBRID (Genişletme Planı)
- [x] **T41** — **Home City & Reinforcement:** `homecity.h/c` modüllerini oluştur. Medeniyet Seviyesi (CivLevel), Sevkiyat Puanı (ShipmentPoints), Refah Puanı (Prosperity) sistemini entegre et. Kart/menü UI ile takviye (Archers, Wood, Tech Upgrade) sistemini kodla.
- [x] **T42** — **Isometric View & Siege Mechanics:** Koordinat sistemini izometrik matrise çevir. `GetIsometricMousePos()` fonksiyonunu ekle. Dev sur yapılarını (Wall Segment) çizim/harita algoritmalarına entegre et. Unit Stations (Archer/Cannon sur üstü, Cavalry sur arkası) yerleştirme mantığını kur. "Kıta" (Horde/Battalion) tabanlı düşman mantığını kodla.
- [x] **T43** — **Base Building (Waves Interval):** `PREP_PHASE` durumu ekle. Haritaya Barracks, Market, Barricade gibi binaların inşa edilmesini sağlayan UI ve yerleştirme sistemini yap.
- [x] **T44** — **Dungeon Mode (RPG/Farming):** `STATE_DUNGEON` durumunu ekle. Hero'nun (WASD/Mouse ile) serbest hareket ettiği, Dungeon düşmanlarının spawn olduğu mantığı yaz. Loot düşürme (Pot, Rune, Gear) ve Envanter entegrasyonunu yap.
- [x] **T45** — **Data Flow & Documentation:** Veri akış diyagramlarını oluştur/belgele, teknik entegrasyonların sorunsuz çalıştığını (Home City → TD Map → Dungeon arası geçişler) doğrula ve test et.

---

## AŞAMA 12 — HERO RPG SİSTEMİ (guidance.md §3)

- [x] **T52** — **Hero Sınıf Seçimi:** Menüye Warrior/Mage/Archer seçim ekranı ekle. `HeroClass` enum + sınıfa göre başlangıç stat tablosu (HP/Mana/Atk/Def/Speed). Seçim ana menüden LOADING'e geçişte kalıcı olsun.
- [x] **T53** — **Gelişmiş Hero Struct:** Mevcut `Hero`'yu genişlet: `HeroClass`, `HeroState`, `HeroStats` (base + current), XP/Level sistemi (`AddXP`, `xpToNext`), `comboCount/comboTimer`, `invulnTimer`, `bodyAngle`, animasyon alanları.
- [x] **T54** — **Skill Sistemi (Q/W/E/R):** `Skill` struct tanımla. Her sınıf için 4 aktif + 2 pasif skill. Q/W/E/R ile kullanım, mana kontrolü, cooldown, AoE/single-target ayrımı. Skill cooldown overlay HUD'da göster.
- [x] **T55** — **Boss Sistemi:** `Boss` struct + `BossPhase` + `BossState` state machine. Her 10 dalgada boss wave. 3 faz geçişi (HP %60/%25). Özel saldırılar (radyal mermi, meteor, minion çağırma). Boss HP bar + nabız/glow çizimi.

## AŞAMA 13 — GÖRSEL EFEKTLERİN GELİŞTİRİLMESİ (guidance.md §8)

- [x] **T56** — **ScreenShake Sistemi:** `ScreenShake` struct. `intensity/decay/offset`. Boss faz geçişi, büyük patlama, level up tetikleyicileri. `BeginDrawing` öncesi kamera offset'e uygula.
- [x] **T57** — **Floating Damage Numbers:** `FloatingText` struct (MAX 40). Hasar, kritik, iyileşme sayıları yukarı kayarak solar. Kritik → sarı + büyük font + "!". Tower'dan ve Hero'dan tetiklenir.
- [x] **T58** — **Mermi Trail Efekti:** `Projectile` struct'a `trailPositions[8]` + `trailIndex` ekle. Her frame geçmiş pozisyon kaydet. Azalan opaklık ve boyutla kuyruk çiz.

## AŞAMA 14 — GAMEPLAY DERİNLİĞİ (guidance.md §9, §7, §4)

- [x] **T59** — **Kule Sinerji Sistemi:** Kule yerleştirilirken 120px içindeki diğer kuleleri tara. Tip çiftlerine göre bonus uygula (BASIC+BASIC → +15% atış hızı vb.). Sinerji çiftleri arası soluk mavi çizgi çiz.
- [x] **T60** — **Harita Etkileşimleri:** `Interactable` struct + `InteractableType` enum. Hazine sandığı, sunak, iyileştirme kuyusu. Hero yaklaştığında tooltip, E ile etkileşim. Haritaya sabit konumlarda yerleştir.
- [x] **T61** — **Gelişmiş Envanter UI:** Mevcut `Inventory`'yi 12 slot grid'e yükselt. `Item` struct (tip, adet, hotkey). `I` tuşu ile panel aç/kapat. Shift+1-6 hızlı kullanım. Slot tooltip.

## AŞAMA 15 — GENİŞLETİLMİŞ İÇERİK (guidance.md §13, §10, §12)

- [x] **T62** — **20 Dalga Sistemi + Boss Dalgaları:** `InitWaves`'i 20 dalgaya genişlet (guidance §13.2 tablosu). Dalga 10 ve 20'de boss wave flag. `isBossWave`, `enemyHpMultiplier`, `waveName` alanlarını Wave struct'a ekle.
- [x] **T63** — **Kamera Sistemi:** `GameCamera` struct. Raylib `Camera2D`. Scroll ile zoom (0.5×–2.0×). H tuşu hero takip toggle. Tüm oyun çizimi `BeginMode2D/EndMode2D` içine al.
- [x] **T64** — **Ses Sistemi Stub:** `AudioManager` struct. `InitAudioDevice()`, `SFXType` enum. Ses dosyası yoksa sessiz devam et. Kule atış, ölüm, level up, dalga başlangıcı hookları.

## Durum Özeti (Aşamalar 12-15)

| Aşama                        | Toplam | Tamamlanan |
|------------------------------|--------|------------|
| 12 — Hero RPG Sistemi        | 4      | 4          |
| 13 — Görsel Efektler         | 3      | 3          |
| 14 — Gameplay Derinliği      | 3      | 3          |
| 15 — Genişletilmiş İçerik    | 3      | 3          |
| **Toplam (yeni)**            | **13** | **13**     |

## AŞAMA 16 — RTS MEKANİKLERİ, KAMERA & FOG OF WAR (UI_DESIGN.md)

- [ ] **T65** — **RTS Seçim Kutusu:** Sol tık sürükleme ile dikdörtgen seçim kutusu. Kutu içindeki dost birimler `selected = true`. Seçili birimlere beyaz halka.
- [x] **T66** — **ESC ile Kule/Bina İptali:** ESC önce kule/bina seçimini iptal eder, yoksa duraklatır. 1/2/3 kule, 4/5/6 bina seçimi.
- [x] **T67** — **Kırsal Alan Binaları:** `CELL_RURAL` hücre tipi. Kışla/Pazar/Town Center binaları 4/5/6 tuşlarıyla rural alanlara kurulabilir.
- [x] **T68** — **Büyük Harita + Köy:** 120×80 grid (CELL_SIZE=12). S-kıvrımlı uzun yol, sonunda 6×6 köy bloğu. `CELL_VILLAGE` hücre tipi + mini ev çizimi.
- [x] **T69** — **Yakın Kamera + WASD Pan:** Default zoom 1.8×, min 1.2× max 3.0×. WASD ve kenar kaydırma ile pan. Harita sınır kontrolü.
- [x] **T70** — **Minimap + Fog of War:** Sol alt 200×130px minimap, kamera viewport çerçevesi. Fog: kule/birim/hero etrafı aydınlık, geri kalan karanlık. Fogdaki düşmanlar çizilmez ve vurulamaz.

| Aşama                          | Toplam | Tamamlanan |
|--------------------------------|--------|------------|
| 16 — RTS/Kamera/Fog            | 6      | 5          |

