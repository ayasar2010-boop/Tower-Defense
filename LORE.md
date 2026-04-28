# LORE — Tower Defense / TD-RPG-Strategy Hybrid

> **Kapsam:** Bu dosya **dünya, hikaye, karakter, senaryo ve tasarım kararlarını** içerir. Teknik görevler için → [`TASKS.md`](TASKS.md).
>
> Bu doküman geliştiricinin değil, **anlatı tasarımcısının** birincil kaynağıdır. Her tasks.md görevinde "→ LORE.md" referansı varsa, geliştirici buradaki metinleri/numaraları/tabloları temel alır.

---

## 📖 İÇİNDEKİLER

1. [Evrenin Özeti — Yüksek Konsept](#1-evrenin-özeti)
2. [Bölgeler ve Haritalar](#2-bölgeler-ve-haritalar)
3. [Hizipler ve Kahramanlar](#3-hizipler-ve-kahramanlar)
4. [Düşman Soyları — "Kadimler"](#4-düşman-soyları)
5. [Ekipman Nadirliği ve Flavor Text](#5-ekipman-nadirliği)
6. [NPC'ler ve Replikler](#6-npcler)
7. [Lore Scrolls — Toplanabilir Parşömenler](#7-lore-scrolls)
8. [Yan Görevler ve Senaryolar](#8-yan-görevler)
9. [Sonlar (Endings)](#9-sonlar)
10. [Tasarım Tabloları (kule sinerjisi vb.)](#10-tasarım-tabloları)

---

## 1. EVRENIN ÖZETİ

### Yüksek Konsept (Plot Twist)
Yüzyıllardır yüzeyde insanlık, **Refah (Prosperity)** uğruna sihir damarlarını (ley hatları) sömürmüştür. Grand Forge'da işlenen her efsanevi silah, dünyanın altındaki dengeyi bir kez daha bozar.

Yüzeye çıkan canavarlar — **Kadimler / İlkdoğanlar (The Firstborn / Deep-born)** — aslında dünyanın bağışıklık sistemidir. Onlar saldırgan değil, **savunmacı**dır. Oyuncu, kampanya boyunca "kötülüğe" karşı savaştığını sanırken, gerçekte dünyanın iyileşmesini geciktirmektedir.

Bu gerçek, oyuncuya **kademeli olarak** Lore Scrolls aracılığıyla açılır. Final seçimini bu bilgi belirler.

### Tonal Ana Hat
- **İlk yarı (Wave 1–10):** Heroik fantastik. Kahraman, kralı için savaşır.
- **Ortası (Wave 10–15):** Şüphe. Parşömenler "düşmanın" perspektifini sızdırır.
- **Final (Wave 15+):** Trajedi. Kazanmak ya da iyileşmek arasında seçim.

---

## 2. BÖLGELER VE HARİTALAR

| Bölge | Tema | Boss | Liberation Ödülü |
|---|---|---|---|
| **Revara Ormanı** | Yeşil-mavi paleti, sis, dev kökler | **Kök-Bağlı Dev** (Vael'thar) | Doğa rünleri, Yeşil ekipman drop oranı +%20 |
| **Kavar Çölü** | Ocra/altın paleti, kum fırtınası | **Kum Solucanı** (Naghul) | Kum-cam mücevherler, hızlı hareket buff |
| **Khorrim Dağları** | Gri-mavi, kar, taş kuleler | **Buzdaki Hatun** (Sireth) | Demir Cevheri stoku, savunma buff |
| **Ölü Topraklar** | Mor/siyah, Void Rifts | **Çürüyen Kalp** (FINAL) | Endgame, Mythical core |

> Harita adları **`fallenSoldiersCount` tooltip'inde dinamik kullanılır** (T111).
> Örnek: *"Bu Revara topraklarında krallık için 342 can verdi."*

---

## 3. HİZİPLER VE KAHRAMANLAR

### Oynanabilir Sınıflar
| Sınıf | Kahraman Adı (varsayılan) | Köken | Karakter Notu |
|---|---|---|---|
| **Warrior** | Sir Galahad of Korr | Khorrim Dağları | Kralın eski yaveri. Ses tonu: ağır, az konuşur. |
| **Mage** | Lirielle "Sessiz" | Revara Ormanı | Ley hatlarını "duyabilen" tek büyücü. Bu yüzden gerçeği ilk hisseden olur. |
| **Archer** | Tarek of Kavar | Kavar Çölü | Bir kervan ailesinden. Ahlakı en gri olan. |

### NPC Hizipleri
- **Krallık Konseyi** (görev veren, *otorite*) — quest hub.
- **Mavi Kapüşonlular** (*sızdıranlar*) — Lore Scrolls'u onlar yerleştirir.
- **Kadimler / Firstborn** — düşman, dilleri yok ama auralar konuşur.

---

## 4. DÜŞMAN SOYLARI — "KADİMLER"

| Tür | Görsel | Davranış | Lore Notu |
|---|---|---|---|
| **Shadow Wolf** | Kara, kırmızı göz | Hızlı, sürü | İlk uyananlar; aslında orman ruhları. |
| **Stone Golem** | Granit, yavaş | Yüksek HP, splash hasar alır | Dağın "hatırlama" mekanizması. |
| **Sand Wraith** | Yarı saydam, kum | Stealth → ekonomi (Sabotör) | Çölün geçmiş intikamcıları. |
| **Burrower** (T108) | Pulluk başlı | Canı azalınca yeraltına iner, 4s sonra hero yakınında çıkar | "Köklerin uyarısı" |
| **Shielded Shaman** (T108) | Kalkan auralı | Mermi yansıtır | Ley hattı şamanları. |
| **Elite (Director AI)** (T98) | %50 büyük, parlama | 3× güç, özel aura | Director'un cezası. |

> **Önemli:** Düşman lore'u sadece flavorText ve Lore Scrolls'ta açıklanır. Diyalog yok.

---

## 5. EKİPMAN NADİRLİĞİ ve FLAVOR TEXT

### Renk Tablosu (T92)
| Nadirlik | Renk (RGB) | Drop Weight |
|---|---|---|
| Common | `#9aa0a6` (gri) | 50 |
| Uncommon | `#3fb950` (yeşil) | 25 |
| Rare | `#3b82f6` (mavi) | 10 |
| Epic | `#a855f7` (mor) | 3 |
| Mythical | `#f59e0b` (altın/turuncu) | 1 |

### Örnek Flavor Text Bankası
- **Paslı Kılıç** *(Common)*
  > *"Krom'un ordusu bu paslı kılıçla ilk sınır kapısını aşmıştı. Sonra unutuldu."*
- **Kadim Savaşçı Kılıcı +3** *(Rare)*
  > *"Onu kuşananın eli titremez derler. Yalan."*
- **Kan İçen Kılıç** *(Epic — Cursed, T99)*
  > *"+%150 hasar. Saniyede %1 HP kaybı. Düşman kestikçe iyileşir."*
  > *"Bir kez kuşanan, bir daha bırakamaz."*
- **Sireth'in Buz Yüreği** *(Mythical — Boss core)*
  > *"Khorrim'in Hatunu, ölmedi. Sadece bekliyor."*

> Geliştiriciye not: Yukarıdaki örnekler, T90 Tooltip görevi için minimum içerik setidir. Tüm metinler `strings_tr.txt` / `strings_en.txt` üzerinden okunmalı.

---

## 6. NPC'LER ve REPLİKLER

### Usta Torbjorn — Köy Demircisi (T93)
**Karakter:** Huysuz, kısa konuşur, oyuncuyu sürekli azarlar ama gizliden gizliye yardımsever.

**Replik bankası (rastgele birinden seçilir):**
- "Kılıcın teneke gibi, getir de adam edelim!"
- "Yine sen mi? Demir bedava değil."
- "+4 mü? Risk senin, çekiç benim."
- "Bu zırhı sen mi giyiyordun? Vah."

### Kervan Muhafızı — "Kayıp Kervanın İntikamı" görevi sonu (T97)
> *"Üç kardeşim de bu yolda kaldı. Bunu al — hak ettin."*

### Zaman Sunağı'nın Sesi (T115)
> *"Yine geldin. Yine erken. On gün geri sarıyorum — bu sefer hatırla."*

---

## 7. LORE SCROLLS — Toplanabilir Parşömenler (T112)

> 12 parşömen. Sırayla açılmazlar; oyuncu hangisini bulursa o açılır.

| # | Başlık | Bulunduğu Yer | Özet (kısa) |
|---|---|---|---|
| I | "Damarlar Hakkında" | Revara — sandık | Ley hatlarının dünyanın kan damarı olduğu. |
| II | "Krom'un Yanlışı" | Khorrim Boss room | İlk Grand Forge inşasının bedeli. |
| III | "Sessiz'in Mektubu" | Mage başlangıç odası | Lirielle'in büyüyü 'duyduğu' an. |
| IV | "Kadimler Konuşmaz" | Kavar — sandık | Düşmanların aslında kötü olmadığı. |
| ... | ... | ... | ... |
| XII | "Son" | Final boss öncesi | Seçim açıklaması. |

> Tam metinler için bkz. `lore/scrolls/` (gelecek görev T112'de oluşturulacak alt klasör).

---

## 8. YAN GÖREVLER ve SENARYOLAR (T97)

### "Kayıp Kervanın İntikamı"
- **Tetik:** Haritada 3 "Yıkık Kervan Arabası" objesini bul.
- **Ödül:** Elit "Kervan Muhafızı" (dost NPC) + **Rare** kılıç.

### "Lanetli Sunak Ayini"
- **Tetik:** "Unutulmuş Sunak" etrafında 15 düşman öldür.
- **Ödül:** Kalıcı +%5 Hasar **veya** Mythical eşya tarifi.

### "Kusursuz Savunma"
- **Tetik:** 3 dalga hiç can kaybetmeden geç.
- **Ödül:** Kraliyet Destek Sandığı (yüksek seviye iksir + Epic zırh şansı).

> Tüm görevler `QuestManager` tarafından yönetilir (T97).

---

## 9. SONLAR (ENDINGS)

> Bu, eski TASKS.md'deki **T100 (Ahlaki Seçim)** maddesinin tasarım dokümanıdır. Kod tarafı sadece bir state machine ve iki alternatif çıkış sahnesi.

### A. Yıkım Yolu (Annihilation)
- Final boss kesilir.
- Cinematic: altın zırhlı kahraman, ölü bir ormanda yürür.
- Sonuç: İnsanlık yaşar, dünyada sihir/doğa söner. Kahraman yalnız.

### B. Uyum Yolu (Coexistence)
- Final boss konuşmadan diz çöker. Kahraman silahını bırakır.
- Cinematic: Grand Forge yıkılır. Şehirler küçülür. Orman geri gelir.
- Sonuç: Refah biter, dünya iyileşir.

### C. (Gizli) Regresyon Sonsuzluğu
- Sadece Time Loop'u (T115) 3+ kez kullanan oyunculara açık.
- Kahraman seçim yapmadan önce sunağa döner ve "döngüyü kabul eder".

---

## 10. TASARIM TABLOLARI

### 10.1 Kule Sinerji Çiftleri (T77 için eksik kalan tablo)
> 120px taraması içinde aktif çiftler:

| Çift A | Çift B | Bonus | Görsel |
|---|---|---|---|
| Basic | Basic | +%15 atış hızı | Soluk mavi çizgi |
| Basic | Splash | +%10 splash radius | Turuncu çizgi |
| Splash | Slow | Slow süresi +1s | Cyan çizgi |
| Slow | Slow | Slow yığılır (max %60) | Beyaz çizgi |
| Splash | Splash | +%20 splash damage | Kırmızı çizgi |
| Sniper *(future)* | Basic | +%25 menzil (Basic'e) | Yeşil çizgi |

### 10.2 Boss Faz Tablosu (T65)
| Faz | HP | Saldırı seti |
|---|---|---|
| 1 | %100 → %60 | Tek hedef yumruk, yavaş yürüyüş |
| 2 | %60 → %25 | Radyal mermi salvası (8-yön) |
| 3 | %25 → 0 | Meteor + minion çağırma + sersemletme auralı koşu |

### 10.3 Element Dokuması Kombinasyonları (T104)
| Sıra | Sonuç | Hasar tipi |
|---|---|---|
| QQQ (3×Ateş) | **Meteor** | AoE yanma |
| WWW | **Buz Tufanı** | AoE slow + dmg |
| EEE | **Arcane Patlama** | True damage |
| QQW | **Buhar Patlaması** | Knockback |
| WWE | **Dondurucu Patlama** | Stun |
| QWE | **Triadik Yıkım** | Yüksek burst |
| QWW | **Buz Bıçağı** | Pierce + bleed |
| WEE | **Mana Buzu** | Düşman manası çal |
| QEE | **Alev Yılanı** | Homing AoE |
| WQE | **Element Kaosu** | RNG sonuç |

---

## 📌 LORE YAZIM KURALLARI

1. **Tüm metinler i18n'den okunur** (T59). Hardcoded string yasak.
2. **Düşmanlar konuşmaz.** Sadece aurala/animasyon ile niyet aktarır.
3. **Oyuncu yavaş öğrenir.** İlk 5 dalgada hiç scroll düşmesin.
4. **Tonal tutarlılık:** Heroik → Şüpheli → Trajik. Karakter komik repliği sadece NPC'lerde.
5. **Cause-of-death log'ları** (T110) ölen birimin son hasar kaynağına göre dinamik üretilir, ama kalıp 8-12 örnekle başlasın.
