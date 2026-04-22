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
- [ ] **T38** — `Makefile` doğrula, `make` ile temiz derleme
- [ ] **T39** — Oynanabilirlik testi: Tüm dalgalar, kule yerleştir/yükselt, game over/victory
- [ ] **T40** — Bellek / sınır kontrolü: Dizi overflow yok, grid sınır aşımı yok

## BONUS (İsteğe Bağlı)
- [ ] **B01** — Kule satma (sağ tık menüsü, %50 geri)
- [ ] **B02** — Yol yön okları
- [ ] **B03** — Dalga ilerleme çubuğu (mini progress bar)
- [ ] **B04** — Kule ateş flash animasyonu

---

## Durum Özeti
| Aşama | Toplam | Tamamlanan |
|-------|--------|------------|
| 1 — İskelet      | 5  | 0 |
| 2 — Harita       | 5  | 0 |
| 3 — Düşman       | 3  | 0 |
| 4 — Kule         | 6  | 0 |
| 5 — Mermi/Partik | 6  | 0 |
| 6 — Dalga        | 4  | 0 |
| 7 — HUD/Girdi    | 3  | 0 |
| 8 — Ekranlar     | 5  | 0 |
| 9 — Test         | 3  | 0 |
| **Toplam**       | **40** | **0** |
