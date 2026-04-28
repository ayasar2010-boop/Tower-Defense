# TASKS — Tower Defense / TD-RPG-Strategy Hybrid

> **Kapsam:** Bu dosya **sadece teknik görevleri** içerir. Hikaye, evren, karakter ve senaryo tasarımı için → [`LORE.md`](LORE.md). Sürüm hedefleri için → [`MVP_SCOPE.md`](MVP_SCOPE.md).
>
> **Kurallar:**
> - Her görev `[ ]` veya `[x]` ile işaretlenir.
> - Risk etiketleri: `[RISK: LOW]` `[RISK: MED]` `[RISK: HIGH]`
> - Bağımlılıklar `→` ile belirtilir (örn. `→ T15`).
> - Her 5 görevde bir **playtest** görevi vardır.

---

## 📊 GENEL DURUM

| Faz | Aşama | Görev Sayısı | Tamamlanan |
|-----|-------|:---:|:---:|
| **MVP-Core** | 1–9 (TD temel) | 40 | 40 |
| **MVP-Core** | 10 (VS Code) | 6 | 6 |
| **MVP-Core** | 11 (Mimari Refaktör) | 4 | 3 |
| **MVP-Core** | 12 (Altyapı: Save / Settings / i18n) | 6 | 0 |
| **MVP-Core** | 13 (Hero RPG) | 4 | 1 |
| **MVP-Core** | 14 (Görsel Efektler) | 3 | 3 |
| **MVP-Core** | 15 (Kamera + Ses) | 2 | 2 |
| **MVP-Core** | 16 (TD-RPG Hybrid) | 5 | 5 |
| **MVP-Plus** | 17 (Gameplay Derinliği) | 3 | 1 |
| **MVP-Plus** | 18 (Dalga & İçerik) | 3 | 3 |
| **MVP-Plus** | 19 (Otonom Şehir) | 5 | 0 |
| **MVP-Plus** | 20 (Loot & Ekipman) | 4 | 0 |
| **MVP-Plus** | 21 (Demircilik) | 4 | 0 |
| **Post-MVP** | 22 (Görevler & Director AI) | 4 | 0 |
| **Post-MVP** | 23 (Yaşayan Evren) | 4 | 0 |
| **Post-MVP** | 24 (Derin Hero Mekanikleri) | 5 | 0 |
| **Post-MVP** | 25 (Müttefik & Şehitlik) | 3 | 0 |
| **Post-MVP** | 26 (Lore Entegrasyonu) | 4 | 0 |
| **Polish** | 27 (Tutorial / Erişilebilirlik / Perf) | 5 | 0 |
| **DLC/Future** | 28 (Co-op Multiplayer) | 5 | 0 |
| **TOTAL** | | **123** | **67** |

---

# ✅ MVP-CORE (1.0 sürümü için zorunlu)

## AŞAMA 1 — İskelet ve Temel Yapı
- [x] **T01** — Dizin yapısı (`src/`, `assets/`)
- [x] **T02** — `Makefile` (Linux + Windows)
- [x] **T03** — `src/main.c` iskeleti
- [x] **T04** — Tüm struct tanımları (Enemy, Tower, Projectile, Particle, Wave, Game)
- [x] **T05** — `main()` game loop iskeleti

## AŞAMA 2 — Harita ve Navigasyon
- [x] **T06** — `InitMap()` 20×12 grid
- [x] **T07** — `InitWaypoints()`
- [x] **T08** — `DrawMap()`
- [x] **T09** — Grid çizgisi toggle
- [x] **T10** — `GridToWorld()` / `WorldToGrid()`

## AŞAMA 3 — Düşman Sistemi
- [x] **T11** — `SpawnEnemy()`
- [x] **T12** — `UpdateEnemies()` (waypoint, slow, lives)
- [x] **T13** — `DrawEnemies()` (tip şekli + HP bar)

## AŞAMA 4 — Kule Sistemi
- [x] **T14** — `CanPlaceTower()`
- [x] **T15** — Kule yerleştirme
- [x] **T16** — `FindNearestEnemy()`
- [x] **T17** — `UpdateTowers()`
- [x] **T18** — `DrawTowers()`
- [x] **T19** — Kule yükseltme (sağ tık)

## AŞAMA 5 — Mermi ve Parçacık Sistemi
- [x] **T20** — `CreateProjectile()`
- [x] **T21** — `UpdateProjectiles()`
- [x] **T22** — `DrawProjectiles()`
- [x] **T23** — `SpawnParticles()`
- [x] **T24** — `UpdateParticles()`
- [x] **T25** — `DrawParticles()`

## AŞAMA 6 — Dalga Sistemi
- [x] **T26** — `InitWaves()` (10 dalga)
- [x] **T27** — `UpdateWaves()`
- [x] **T28** — Dalga tamamlanma → `STATE_WAVE_CLEAR`
- [x] **T29** — Son dalga → `STATE_VICTORY`

## AŞAMA 7 — HUD ve Girdi
- [x] **T30** — `DrawHUD()`
- [x] **T31** — Yerleştirme önizlemesi
- [x] **T32** — `HandleInput()`

## AŞAMA 8 — Durum Ekranları
- [x] **T33** — `DrawMenu()` + `UpdateMenu()`
- [x] **T34** — `DrawPauseOverlay()` + `UpdatePause()`
- [x] **T35** — `DrawGameOver()`
- [x] **T36** — `DrawVictory()`
- [x] **T37** — `InitGame()` / `RestartGame()`

## AŞAMA 9 — Derleme ve Test
- [x] **T38** — `Makefile` doğrulama
- [x] **T39** — Oynanabilirlik testi
- [x] **T40** — Bellek / sınır kontrolü

### Bonus (tamamlandı)
- [x] **B01** — Kule satma
- [x] **B02** — Yol yön okları
- [x] **B03** — Dalga ilerleme çubuğu
- [x] **B04** — Kule ateş flash animasyonu

---

## AŞAMA 10 — VS Code Game Studio Ortamı
- [x] **T46** — `.vscode/extensions.json`
- [x] **T47** — `.vscode/settings.json`
- [x] **T48** — `.vscode/c_cpp_properties.json`
- [x] **T49** — `.vscode/tasks.json`
- [x] **T50** — `.vscode/launch.json`
- [x] **T51** — `CLAUDE.md` güncelleme

---

## ⚠️ AŞAMA 11 — MİMARİ REFAKTÖR `[RISK: MED]`
> **Bu aşama Aşama 16'dan ÖNCE tamamlanmalıdır.** Tek `main.c` ile homecity/dungeon/network gibi sistemleri yönetmek imkansıza yakın. Borç birikmesin.

- [x] **T52** — **Modülerleştirme:** Tek `main.c`'yi parçala. Hedef yapı:
  - `src/main.c` (sadece game loop + state switch)
  - `src/map.c/h` `src/enemy.c/h` `src/tower.c/h` `src/projectile.c/h`
  - `src/particle.c/h` `src/wave.c/h` `src/hud.c/h`
  - `src/input.c/h` `src/util.c/h` (vec math, RNG)
- [x] **T53** — **Nesne Havuzu (Object Pooling):** Mermi, partikül, düşman, floating-text için statik diziler + `isActive` bayrağı. `malloc/free` yasak.
- [x] **T54** — **Tip ortak başlığı (`types.h`):** Tüm `enum` ve struct'ları tek yerden include edilebilir kıl. Forward declaration disiplini.
- [ ] **T55** — **🎮 PLAYTEST-1:** Refaktör sonrası tüm Aşama 1–9 fonksiyonu regresyonsuz çalışıyor mu? Kabul kriteri: 60 FPS'de 200 düşman + 50 kule + 300 mermi.

---

## 🆕 AŞAMA 12 — ALTYAPI: Save / Settings / Lokalizasyon `[RISK: MED]`
> Bu aşama **Hero RPG ve Co-op için ön koşul**. Erkene çekildi (eski T87 regresyon, T101 multiplayer karakter taşıma bunu zaten gerektiriyor).

- [x] **T56** — **Save/Load Sistemi:** `fwrite`/`fread` ile slot tabanlı kayıt (3 slot). `Game` struct'ı pointer-free; pointer yerine pool indeksleri kullan. Versiyonlama header'ı (magic + version).
- [x] **T57** — **Ayarlar Menüsü:** Çözünürlük, fullscreen, master/SFX/music ses seviyeleri. `settings.ini` dosyası.
- [ ] **T58** — **Key Rebinding:** Tüm input'lar `keymap[]` üzerinden okunsun. Ayarlar menüsünden değiştirilebilsin.
- [ ] **T59** — **Lokalizasyon altyapısı:** `strings_tr.txt` + `strings_en.txt`. Her UI metni `T("KEY")` makrosundan geçsin. Eski hardcoded string yasak.
- [x] **T60** — **Logger:** `LOG_INFO`, `LOG_WARN`, `LOG_ERR` makroları. Dosyaya + stdout'a yaz.
- [ ] **T61** — **🎮 PLAYTEST-2:** Save→Quit→Load döngüsü, dil değiştirme, key rebind. Hiçbir ayar kaybolmamalı.

---

## AŞAMA 13 — Hero RPG Sistemi
- [x] **T62** — Hero Sınıf Seçimi (Warrior/Mage/Archer)
- [x] **T63** — Gelişmiş Hero Struct (HeroState, HeroStats, XP/Level, combo, invuln, bodyAngle)
- [x] **T64** — **Skill Sistemi (Q/W/E/R):** Her sınıf 4 aktif skill. Mana, cooldown, AoE/single. Alt HUD overlay. → T62
- [x] **T65** — **Boss Sistemi:** isBoss/bossPhase Enemy'de. Her 10 dalgada boss. 3 faz (HP %60/%25). Radyal mermi, meteor, minion. Boss HP bar + nabız.

---

## AŞAMA 14 — Görsel Efektler
- [x] **T66** — ScreenShake (toggle erişilebilirlik için **T108**'de eklenecek)
- [x] **T67** — Floating Damage Numbers
- [x] **T68** — Mermi Trail Efekti

---

## AŞAMA 15 — Kamera + Ses (eskiden 15-16'daydı, erkene alındı)
> **Bağımlılık düzeltmesi:** Kamera, T80 (izometrik) tarafından gerekli. Ses, durum ekranlarından beri eksikti.

- [x] **T69** — **Kamera Sistemi:** `GameCamera` (Camera2D). Scroll zoom (0.5×–2.0×). H tuşu hero takip. Tüm çizim `BeginMode2D/EndMode2D` içine.
- [x] **T70** — **Ses Sistemi:** `AudioManager`, `InitAudioDevice()`, `SFXType` enum. Eksik dosyada sessiz devam. Tüm hooks: kule atış, ölüm, level up, dalga başlangıcı, hero damage, UI click.

---

## AŞAMA 16 — TD-RPG-Strategy Hybrid `[RISK: HIGH]`
> Bu aşamadan itibaren oyun gerçek bir hibrit hal alır. **Aşama 11 refaktörü tamamlanmadan başlama.**

- [x] **T71** — Home City & Reinforcement (`homecity.c/h`, CivLevel, ShipmentPoints, Prosperity)
- [x] **T72** — Isometric View & Siege Mechanics (Wall Segment, Position Slots, Horde mantığı)
- [x] **T73** — Base Building (`PREP_PHASE`, Barracks, Market, Barricade)
- [x] **T74** — Dungeon Mode RPG (`STATE_DUNGEON`, hero free-move, loot drop, envanter)
- [x] **T75** — Data Flow & Documentation
- [ ] **T76** — **🎮 PLAYTEST-3:** Home City → TD Map → Dungeon geçişleri kayıpsız çalışıyor mu?

---

# 🔷 MVP-PLUS (1.1 sürümü)

## AŞAMA 17 — Gameplay Derinliği
- [x] **T77** — **Kule Sinerji Sistemi:** 120px taraması. ⚠️ **Tasarım kararı eksik** — sinerji çiftleri tablosu için `LORE.md`'ye bak ya da bu görevi belge tamamlanana kadar bekleme moduna al.
- [x] **T78** — **Harita Etkileşimleri:** `Interactable` struct + enum (sandık, sunak, kuyu). Hero yaklaşınca tooltip + E. (dungeon.c'de implemente)
- [x] **T79** — **Gelişmiş Envanter UI:** 12 slot grid, `Item` struct, I tuşu, Shift+1-6 hızlı kullanım, slot tooltip. (dungeon.c'de implemente)

## AŞAMA 18 — Dalga & İçerik Genişletme
- [x] **T80** — 20 Dalga Sistemi + Boss Dalgaları
- [x] **T81** — *(eski T63, kameraya taşındı)* — `T69` ile birleşti
- [x] **T82** — *(eski T64, sese taşındı)* — `T70` ile birleşti

## AŞAMA 19 — Otonom Şehir ve Ekonomi
- [ ] **T83** — **Köylü FSM ve Outposts:** IDLE → MOVE_TO_NODE → BUILD_CAMP → GATHERING → TRANSPORT_TO_TOWN. Surdışı kamplar savunmasız.
- [ ] **T84** — **Kademeli Sur Sistemi:** Prosperity eşikleri T1(odun)→T2(taş)→T3(güçlendirilmiş). `PositionSlot[]` snap, range/dmg bonus.
- [ ] **T85** — **Auto-Train & Patrol:** Askeri binalara toggle. Üretilen birim `FUNIT_PATROL` → Rally Point.
- [ ] **T86** — **Manager Mimarisi:** `EconomyManager`, `BuildingManager`, `UnitManager`. Tick-rate optimizasyonu (AI 4-5 Hz). → T52
- [ ] **T87** — **Global Terrain (Perlin/Simplex):** Çamur (-%30 hız), Uzun Ot (gizlenme), Taşlık (+zırh).

## AŞAMA 20 — Loot & Ekipman Sistemi
- [ ] **T88** — **Kaynak Muhafızları:** Tarafsız muhafız canavarlar. Kaynak değerine göre seviye. Pending Loot.
- [ ] **T89** — **End-Level Loot Chest:** Sandık animasyonu + nadirlik renkleriyle liste.
- [ ] **T90** — **Detaylı Tooltip:** Görsel, isim, nadirlik, statlar, flavor. → T59 (i18n)
- [ ] **T91** — **Hero Ekipman Slotları + Drag&Drop:** Paperdoll (Weapon/Armor/Head/Accessory). Stat anında yansır.

## AŞAMA 21 — Demircilik ve Yükseltme `[RISK: MED]`
- [ ] **T92** — **Nadirlik (Rarity) Sistemi:** Common/Uncommon/Rare/Epic/Mythical. `flavorText` alanı. → LORE.md
- [ ] **T93** — **Blacksmith Binası + NPC:** `BUILDING_BLACKSMITH`. Forge UI. NPC karakter ve replikleri için → LORE.md.
- [ ] **T94** — **Eşya Yükseltme +1/+2/+3:** Demir Cevheri, Büyülü Toz. +3'e %100, +4'ten sonra RNG. Başarısızlıkta -1, kırılma yok. Glow efekti.
- [ ] **T95** — **Grand Forge (Home City):** Boss core'larından "Mythical" üretim.

---

# 🟣 POST-MVP (1.2+ sürümleri)

## AŞAMA 22 — Görevler ve Director AI
- [ ] **T96** — **Weighted Loot Tables:** Her düşman + Boss için `dropChance` + `dropWeight`.
- [ ] **T97** — **QuestManager:** Dinamik yan görevler. Senaryolar için → LORE.md.
- [ ] **T98** — **Director AI:** Performans takibi (can/altın/temizleme süresi). Flawless oynayana Elite düşmanlar.
- [ ] **T99** — **Cursed Loot:** Çift taraflı eşyalar. Vignette/aura efekti. **Bağımlılık:** T98 olmadan etkisiz, → T98.

## AŞAMA 23 — Yaşayan Evren
- [ ] **T100** — **Living Time:** Sürekli zaman akışı + Day/Night cycle.
- [ ] **T101** — **Diegetic UI:** Dungeon Entrance binası, Courier Post (atlı ulak).
- [ ] **T102** — **Background Defense Sim:** Kümülatif `totalTowerDPS`/`totalSoldierCount`. 10s snapshot. Snap-back. Kritik uyarı.
- [ ] **T103** — **İzometrik Zindanlar:** Top-down → İzometrik. Özel canavar yetenekleri (ışınlanma, zehir bulutu, skill blok).

## AŞAMA 24 — Derin Hero Mekanikleri
- [ ] **T104** — **Mage Element Dokuması:** 3 slotlu kuyruğa Q/W/E ekle, R çağır. 10 büyü kombinasyonu. → T64
- [ ] **T105** — **Warrior Öfke & Cleave:** Koni AoE (%50 sekme). Aura + müttefik defense buff.
- [ ] **T106** — **Archer Wind Run + Pin Arrows:** %100 evasion, no-collision. Pin → arkadaki cisme it + 3s stun.
- [ ] **T107** — **Tavern & Mercenaries:** Otonom yoldaş kiralama. FSM (PATROL/ENGAGE/HEAL).
- [ ] **T108** — **Smart Enemies:** Behavior Tree. Tünelci (burrow), Kalkanlı Şaman (mermi yansıtma), Sabotör (stealth → ekonomi binaları).

## AŞAMA 25 — Müttefik ve Şehitlik
- [ ] **T109** — **Ally Requisitions:** Talep bildirimi, Accept/Reject.
- [ ] **T110** — **Dynamic Graveyards:** Cause-of-death log + tombstone tooltip.
- [ ] **T111** — **Fallen Soldiers Monument:** `fallenSoldiersCount` global. Bölgesel tooltip.

## AŞAMA 26 — Lore Entegrasyonu
> Bu görevler `LORE.md` ile sıkı koordine.
- [ ] **T112** — **Lore Scrolls:** Toplanabilir parşömenler. Metinler için → LORE.md.
- [ ] **T113** — **Fog of War Dungeon Discovery:** Zindan kapısını fiziksel keşif şart.
- [ ] **T114** — **Regional Bosses:** Orman/Çöl/Dağ özel boss + Liberation flag.
- [ ] **T115** — **Time Loop / Regression `[RISK: HIGH]`:** Hero ölünce Shrine of Time. 10 gün geri. Stats + envanter korunur. → T56 (save serialization).

---

# 🟢 POLISH (1.3 — 1.0 öncesi son cila)

## AŞAMA 27 — Polish & QoL
- [ ] **T116** — **Tutorial / Onboarding:** İlk oyunda interaktif rehber. 5-7 adım. Skip butonu.
- [ ] **T117** — **Erişilebilirlik:** Renk körü modu (3 preset), font scale, **screen shake toggle**, kritik metinler için kalın font.
- [ ] **T118** — **Performance Profiler:** Frame time, draw call, aktif entity sayacı. F3 ile aç.
- [ ] **T119** — **Crowd Control Çeşitliliği:** slow/stun mevcut. Knockback, fear, silence, root ekle.
- [ ] **T120** — **🎮 PLAYTEST-FINAL:** Tutorial → 20 dalga → Boss → Dungeon → Victory. Fresh user testleri.

---

# 🔮 DLC / FUTURE

## AŞAMA 28 — Co-op Multiplayer `[RISK: HIGH]` `[POST-LAUNCH]`
> **Tek başına 3-6 ay iş.** Singleplayer 1.0 çıkmadan başlama. Ayrı feature branch.

- [ ] **T121** — Listen Server (ENet), karakter taşıma (JSON), 5 oyuncu lobi. → T56
- [ ] **T122** — State Prediction + Interpolation + Delta Compression.
- [ ] **T123** — Blueprint + Host Onay Mekanizması. `PendingBuildings[]`. Pathfinding bypass.
- [ ] **T124** — Dynamic Difficulty (`ActivePlayerCount` çarpanları), Aggro algoritması.
- [ ] **T125** — **Anti-cheat + Reconnection + Voice Ping/Quick Chat.** (Bu görev eski plana ek olarak eklendi.)

---

# 📌 KESİLEN / ERTELENEN ÖĞELER

| Eski ID | Görev | Karar | Sebep |
|---|---|---|---|
| T82 (eski) | Void Rifts | **T98 sonrasına ertelendi** | Director AI olmadan anlamsız |
| T99 (eski) | Çoklu Son / Ahlaki Seçim | **Lore'a taşındı** (`LORE.md` §Endings) | Tasarım kararı, kod görevi değil |
| T96-T98 (eski) | Lore detayları | **Lore'a taşındı** | Bunlar yazım, kod değil |
| T59 (eski) | Kule Sinerji çiftleri | **Tasarım dokümanına taşındı** | Tablo eksikti |

---

# 🔗 BAĞIMLILIK GRAFİĞİ (özet)

```
T52-T55 (Refaktör)  ─┬─→ T56-T61 (Altyapı) ─┬─→ T62-T65 (Hero RPG)
                     │                       └─→ T71-T76 (Hybrid)
                     └─→ T86 (Manager)        ↓
                                              T77-T95 (MVP-Plus)
                                              ↓
T56 (Save) ─────────────────────────→ T115 (Time Loop) + T121 (Multiplayer)
T98 (Director) ──────────────────────→ T99 (Cursed Loot)
T64 (Skills) ────────────────────────→ T104-T106 (Hero revamp)
T70 (Audio) ─────────────────────────→ T119 (CC çeşitliliği)
```
