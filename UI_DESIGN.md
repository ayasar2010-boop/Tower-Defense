# GRAND SIEGE: UI & UX DESIGN SPECIFICATION

This document outlines the unified high-fidelity interactive prototype design for "Grand Siege" — merging the dark-fantasy aesthetics and web-based tech stack (React + Tailwind + Canvas2D) with the core gameplay features of the "Ruler" TD-RPG framework. 

## 1. TECH STACK & ARCHITECTURE
- **Core:** Single HTML entry with three main React modules: `map-engine.jsx`, `ui-panels.jsx`, `app.jsx`.
- **UI Framework:** React + TailwindCSS + lucide-react (for iconography).
- **Rendering:** Canvas2D for the organic world, tile painting, and particle effects.
- **Theme/Styling:** Obsidian dark-fantasy tailored through Tailwind arbitrary values and custom layers.

---

## 2. ART DIRECTION & AESTHETIC
- **Palette:** 
  - *Obsidian (Base):* Deep blacks (`#07070a`, `#0c0d12`, `#14151c`).
  - *Antique Gold (Accents):* `#c89b3c`, `#d9b056`, `#e8c777`.
  - *Hero & Magic:* Emerald hero tones (`#3fa07a`, `#5dc199`), Magenta/Pink for boss auras, Cyan/Purple for Mage abilities.
  - *Horde/Enemies:* Blood red (`#7a2a2a`, `#a83a3a`).
  - *Allies/Civilians:* Moss green (`#3fa07a`), Steel blue variants (`#4a8fb8`).
- **Typography:** 
  - *Cinzel:* Titles, ceremonial labels, HUD numbers (tracked wide, 0.05-0.35em).
  - *Inter:* Body text, descriptions, standard UI text.
  - *JetBrains Mono:* Technical data, coordinates, keybinds.
- **Chrome Motifs:** 
  - **Gilded Frames:** Dark gradient background + 1px gold border (35% opacity) + heavy drop shadow + 14x14 absolute-positioned corner brackets (`.corner.tl/tr/bl/br`).
  - **Gold-Text:** Linear gradient (`#f0d68a`→`#c89b3c`→`#8f6b1f`) with `background-clip: text`.
  - **Runic Dividers:** Thin gold gradient lines flanking small Cinzel texts.
  - **KBD Keycaps:** Amber colored pill-shaped with a 2px bottom border, 11px Mono.

---

## 3. WORLD & MAP RENDERING (CANVAS 2D)
- **Grid:** 200×120 cells (CELL=10px), yielding 24,000 cells managed deterministically.
- **Biomes & Terrain:** Rendered organically as continuous radial-gradient blobs (VOID, GRASS, DIRT, FOREST, ROCK) overlaid with a 4% global film grain. 
- **Landmarks:** 
  - Meandering river (sine-modulated).
  - S-shaped Siege Road.
  - Walled City District with iron gates and golden-glowing central Gilt Market.
- **Interactive Layers:**
  - **Grid Overlay:** Only visible during Tower Placement mode (T key). Show green/red tile previews and pulsing white range rings around placements.
  - **Fog of War:** 78% ambient dusk base. Hero and active towers cut soft radial gradient holes into the fog. Explored areas stay at 55% alpha.
  - **Pathing/Flow Field:** Gold animated arrows moving toward the East Gate. Changes to blood-red when the Horde spawns.

---

## 4. DYNAMIC ROLES & ENTITIES
- **The Hero (The Warden):** Click-to-move entity with emerald glow/vision. Can cast classes-specific skills.
- **The Horde (Enemies):** Normal (Raiders), Fast (Wolf Riders), Tank (Iron Guards), and Bosses (Golem Kings). Triggers red HP bars and floating damage numbers on hit.
- **Civilians:** 90 small dot entities within city walls performing civilian idle pathing.
- **Friendly Units (Reinforcements):** Spawns via Home City shipments or Hero ultimate. Colored visually by type (Blue, Green, Purple).
- **Towers:** Basic (50g), Sniper (100g), Splash (150g). Golden projectiles seek enemies, producing destruction bursts upon impact.

---

## 5. HUD (HEADS-UP DISPLAY)
### Top Bar
- **Left Cluster (Stat Badges):** Gilded frames for Resource text. 
  - *Gold* (Coins/Gold icon)
  - *Lives* (Heart/Blood icon)
  - *Wave X/20* (Swords/Silver icon)
- **Center Cluster (Siege Progress):**
  - Standard Waves: Gilded panel, 420px map progress meter (blood-600 → gold-500). Tick marks at 25/50/75% mapped to "OUTSKIRTS · CROSSROADS · OUTER WALL · INNER KEEP".
  - **Boss Waves (Every 10th Wave):** OVERRIDES progress meter with a massive pulsing Boss HP Bar (Maroon background, Gold border). Indicates Phase 1, Phase 2, Phase 3 and displays Boss Name (e.g., "BOSS: GOLEM KING (PHASE 2)").
- **Right Cluster:** "TRIGGER HORDE" hazard toggle + Hamburger Menu.

### Bottom Overlay
- **Bottom Center (Hero Skills):** 4 Skill Slots (Q / W / E / R) showing Mana Cost overlays and visual Cooldown dials (desaturated/darkened until ready).
- **Bottom Left Context Inspect:** Displays clicked tile, Tower stats, or Enemy details with Roman-numeral threat levels and flavor text.

### Visual Effects
- **Floating Text:** Critical hits sprout large, yellow Inter bold texts with exclamation marks. Heals are green.
- **Screen Shakes:** Canvas global offsets triggered on Boss Phase Transitions (e.g., 60% and 25% HP) and splash blasts.
- **Wave Banner:** Centered gilded card popping out for 2.4s ("WAVE 10 · THE HORDE MARCHES", "NOT ENOUGH GOLD").

---

## 6. MENU & UI PANELS (ESC) 
*ESC stack popping order: placement mode → context inspect → open panel.*

Left nav column (340px widths), Right content area:
1. **THE VAULT / INVENTORY (Package Icon):**
   - 12 slots grid (Replaces legacy array structure). Rarity colored borders.
   - Right Detail Sidebar: Selected Relic / Gear stats (+Vision, +Damage, -Mana), lore text, and EQUIP/DISCARD toggles.
2. **CHRONICLES (Scroll Icon):**
   - Save slots featuring Campaign title, "ACTIVE" badge, Playtime, and Load buttons.
3. **HOME CITY & SHIPMENTS (Castle Icon):**
   - Tech Trees: CivLevel upgrades, Barracks, Markets.
   - Shipment points UI to summon Reinforcement Waves. 
4. **THE ALCHEMIST (Conical Flask Icon):**
   - Graphics & Audio toggles (Low/Ultra, Volume Sliders).
   - Combat Keybinds mapping.
5. **CODEX (Book Icon):**
   - Bestiary lists with skull icons mapping the Raiding forces and Bosses.
6. **CLASS SELECTOR & RETREAT:**
   - Initial Loading Screen to choose between Warrior / Mage / Archer, defining the Hero's base attack radius and the 4 abilities mapped to the bottom skill bar.

---

## 7. INPUT & UX FLOW
- **Camera:** Drag to pan. Mouse Wheel to zoom (0.35x - 2.2x). `H` to center on Hero. `R` to Reset Camera.
- **Interactions:** 
  - Left Click walkable tile = Move Hero. 
  - `T` = Toggle Tower Placement. 
  - Num-keys `1`, `2`, `3` = Select Basic, Sniper, Splash towers.
  - `Q`, `W`, `E`, `R` = Activate Hero Actions (Requires Mana).
  - `G` = Trigger Wave Early.
  - `I` = Quick-open Inventory/Vault.
- **Feedback:** "Cannot Place Here" localized blood-red warnings, "+180 GOLD" notifications rising gracefully. Game-over overlays the world with an 85% obsidian blur requesting players to "BEGIN ANEW".
