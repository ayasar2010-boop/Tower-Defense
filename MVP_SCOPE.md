# MVP_SCOPE — Sürüm Yol Haritası

> Bu doküman **"Ne yaparsak oyunu yayınlayabiliriz?"** sorusunun cevabıdır.
> Teknik görevler → [`TASKS.md`](TASKS.md) | Hikaye → [`LORE.md`](LORE.md)

---

## 🎯 ÜRÜN VİZYONU (TEK CÜMLE)

> *"Tower Defense iskeletine RPG kahramanı, otonom bir şehir ekonomisi ve seçim-temelli bir final eklenmiş, tek-oyunculu, izometrik fantastik bir hibrit oyun."*

Bu cümleye uymayan her özellik **scope dışıdır**.

---

## 📦 SÜRÜM PLANI

| Sürüm | Adı | Hedef Tarih | Kapsam | Aşamalar |
|---|---|---|---|---|
| **0.5** | "Foundation" | T+4 hafta | Refaktör + Altyapı + Hero RPG | A11–A15 |
| **0.7** | "Hybrid" | T+8 hafta | TD-RPG-Strategy hibrit oynanabilir | A16 |
| **0.9** | "Beta" | T+14 hafta | Loot + Demircilik + 20 dalga | A17–A21 |
| **1.0** | **MVP RELEASE** ⭐ | T+18 hafta | Tutorial + Polish + Erişilebilirlik | A27 |
| **1.1** | "Living World" | T+24 hafta | Director AI + Yaşayan Evren + Lore | A22–A26 |
| **1.2** | "Co-op" | T+36 hafta | Multiplayer DLC | A28 |

> Tarihler: tek geliştirici, haftada ~25 saat varsayımıyla.

---

## ⭐ MVP (1.0) — KRİTİK YOLCULUK

### 1.0 İçin Zorunlu Özellikler ("Must Have")

#### Mekanik
- ✅ TD core (kule yerleştir/yükselt/sat, 20 dalga, boss)
- ✅ Hero (3 sınıf, Q/W/E/R skill, level/XP)
- ✅ Home City + Reinforcement
- ✅ Dungeon Mode (en az 1 zindan)
- ✅ Loot + Envanter + Ekipman + Demircilik (+1/+2/+3)
- ✅ Save/Load (3 slot)
- ✅ Ayarlar menüsü + key rebinding
- ✅ TR/EN lokalizasyon

#### İçerik
- ✅ 1 bölge (Revara Ormanı) tam playable
- ✅ 20 dalga + 2 boss (Wave 10, Wave 20)
- ✅ ~30 farklı eşya (5 nadirlik × 6 slot)
- ✅ Tutorial / onboarding
- ✅ 1 son ekranı (Annihilation default)

#### Teknik
- ✅ Modüler kod (`hero.c`, `map.c`, ...)
- ✅ Object pooling (mermi, partikül, düşman, FT)
- ✅ Stable 60 FPS @ 200 düşman + 50 kule
- ✅ Erişilebilirlik temelleri (renk körü modu, screen shake toggle, font scale)
- ✅ Logger + crash dump

### 1.0 İçin "Should Have" (eksikse OK ama tercih edilen)
- 🟡 Mage Element Dokuması (T104) — basit Q/W/E/R yeterli
- 🟡 Cursed Loot (T99)
- 🟡 Tavern & Mercenaries (T107)
- 🟡 Smart Enemies (T108)

### 1.0 İçin **OLMAYACAK** Özellikler ("Won't Have" — net çizgi)
| Özellik | Sebep | Hangi sürümde? |
|---|---|---|
| ❌ Time Loop / Regresyon (T115) | `Game` struct'ı tam serializable yapmak büyük teknik borç | 1.1 |
| ❌ Çoklu Son (Coexistence) | Ekstra cinematic + senaryo dalı | 1.1 |
| ❌ Co-op Multiplayer (A28) | 3-6 ay ek iş, ayrı feature branch | 1.2 |
| ❌ Void Rifts (T99 eski) | Director AI olmadan anlamsız | 1.1 |
| ❌ Background City Sim (T102) | Karmaşıklık çok yüksek, değer/maliyet düşük | 1.1 |
| ❌ Diğer 3 bölge (Çöl/Dağ/Ölü Topraklar) | Konsept doğrulaması için 1 bölge yeter | 1.1 (Çöl), 1.2 (kalanı) |

---

## 🚦 KARAR KRİTERLERİ — "1.0 hazır mı?"

### Çıkış kriterleri (hepsinin geçmesi şart)
- [ ] **PT-1:** Yeni oyuncu, tutorial + 5 dalga'yı 30 dk içinde bitirebiliyor.
- [ ] **PT-2:** Tam oyun (20 dalga + boss + dungeon) 90-120 dk arası.
- [ ] **PT-3:** Save→Quit→Load 3 farklı noktada hatasız.
- [ ] **PT-4:** 1 saat aralıksız oynamada bellek sızıntısı yok (RSS sabit).
- [ ] **PT-5:** Stable 60 FPS @ 200 düşman + 50 kule + 300 mermi (Aşama 11 kabul kriteri).
- [ ] **PT-6:** Renk körü modu açıkken tüm bilgi kanalları okunabiliyor (sadece renkle ayrım yok).
- [ ] **PT-7:** Crash-free, 5 farklı bilgisayarda smoke test.
- [ ] **PT-8:** Tüm UI metinleri TR/EN'de mevcut, eksik string yok.

### Risk Sinyalleri (görüldüyse 1.0'ı erteleyecek)
- 🚨 Refaktör (A11) sonrası belirgin regresyon
- 🚨 Tutorial'da %50+ kullanıcı zorlanıyor
- 🚨 Boss balance: Wave 20 boss <%5 başarı oranı
- 🚨 Hero ölümü → save corruption

---

## 🧭 GELİŞTİRME PRENSİPLERİ

### "Dur ve Düşün" Anlık Sayaçları
Bir özellik eklemeden önce kendine sor:
1. **MVP cümlesine uyuyor mu?** (yukarı bak) — Hayır → kes.
2. **Bu özellik 3 hafta içinde tamamlanır mı?** — Hayır → 1.1'e al.
3. **MVP'nin diğer parçalarını blokluyor mu?** — Hayır → erteleyebilirsin.
4. **Tutorial'da göstermek zorunda mıyım?** — Evet → polish kalitesine getir.

### "Definition of Done" — Her görev için
- [ ] Kod yazıldı, derleniyor.
- [ ] En az 1 manuel oynanış testi yapıldı.
- [ ] Yeni hardcoded string yok (i18n'e eklendi).
- [ ] Yeni `malloc` yok (pooling kullanıldı — T53'ten sonra).
- [ ] `LOG_INFO`/`LOG_WARN` ile observability eklendi.
- [ ] Görev varsa accessibility hookları eklendi (renk + ses + metin).
- [ ] `TASKS.md`'de `[x]` işaretlendi.

---

## 📊 ZAMAN BÜTÇESİ — KABA TAHMİN

| Aşama | Süre | Risk |
|---|---|---|
| A11 Refaktör | 1 hafta | MED |
| A12 Altyapı | 1 hafta | MED |
| A13 Hero RPG | 2 hafta | LOW |
| A14–A15 Efekt+Kamera+Ses | (büyük ölçüde tamam) | — |
| A16 Hybrid | 2 hafta | HIGH |
| A17 Gameplay | 1 hafta | LOW |
| A19 Otonom Şehir | 2 hafta | HIGH |
| A20 Loot | 1.5 hafta | LOW |
| A21 Demircilik | 1 hafta | MED |
| A27 Polish | 2 hafta | MED |
| **TOPLAM 1.0** | **~13.5 hafta net** | |
| Buffer (%30) | +4 hafta | |
| **Hedef** | **18 hafta** | |

---

## 🎓 ALINTI / ÖĞÜT

> "Bin hayır, bir evet için." — `system_guidelines`
>
> Her bir özellikte "Bunu eklemezsek ne kaybederiz?" diye sor.
> Cevap *"hiçbir şey kritik değil"* ise → 1.1'e al.

---

## ✅ HEMEN ŞİMDİ YAPILACAK

1. **Refaktör başlat (A11)** — bu olmadan A16 kâbus olur.
2. **Save/Load yaz (T56)** — A12'nin omurgası.
3. **i18n geç (T59)** — şimdi yapılmazsa sonra her satırı tekrar yazarsın.
4. **PLAYTEST-1'i (T55) kabul kriteri olarak işaretle** — refaktör için kalite kapısı.
