# Getting a map you edited into the game

The game is one HTML file. A map is one JSON file. This is how you get from a
scene you just edited to walking around it.

## The short version

1. Export the map from Unity or Unreal.
2. Open the game HTML file in a browser.
3. Open the browser console and run `B4D_LOAD_MAP(<paste the JSON>)`.
4. It stores the map and reloads. Pick that campaign from the menu and play.

Nothing is uploaded. The map lives in that browser's local storage until you
clear it.

## In full

### 1. Export

**Unity.** Window ▸ Blox 4 Dead ▸ Map Kit ▸ **Export JSON**. It refuses if the
map has errors, so a file that comes out will load.

**Unreal.** Tools ▸ Blox 4 Dead ▸ **Export Campaign JSON**. Same rule.

### 2. Pick a slot

`index` in the map decides which campaign it replaces:

| index | Campaign | Data driven |
| --- | --- | --- |
| 0 | NO MERCY, rail yard | no, hand built |
| 1 | DEPARTURE, airport | no, hand built |
| 2 | BLOOD MEAL, slaughterhouse | yes |
| 3 | DEAD TIDE, harbor | replaceable |
| 4 | BLACKOUT DETOUR, crosstown | replaceable |

Slots 2, 3 and 4 build from data whenever a map exists for them, so set `index`
to one of those. Slots 0 and 1 are still hand built and ignore map data.

Whatever slot you use keeps the built in campaign's name in the menu. A map in
slot 2 is still listed as BLOOD MEAL.

### 3. Load it

Open the game, then open the console:

- Safari: Develop ▸ Show JavaScript Console. Enable the Develop menu first in
  Settings ▸ Advanced.
- Chrome and Edge: View ▸ Developer ▸ JavaScript Console.
- Firefox: Tools ▸ Browser Tools ▸ Web Console.

Then either paste the map inline:

```js
B4D_LOAD_MAP({ "schema": 1, "id": "my_map", "index": 2, ... })
```

or, easier with a big file, copy the whole JSON to the clipboard and paste it
between the brackets of:

```js
B4D_LOAD_MAP(`  `)
```

Both work; the second avoids the console reformatting a long object.

The map is checked before it is stored. Errors are printed and nothing is saved.
Warnings are printed and it loads anyway.

### 4. Play it

The page reloads on its own. Start the game and pick the campaign for the slot
you used.

### Console commands

| Command | What it does |
| --- | --- |
| `B4D_LOAD_MAP(map)` | Checks, stores and loads a map. Reloads the page. |
| `B4D_LIST_MAPS()` | Shows which maps are stored and which slot each holds. |
| `B4D_CLEAR_MAPS()` | Removes them all and puts the built in levels back. |

### Why it reloads

The world is built once when the page starts. Anything set after that is too
late for that run, so the map is stored first and picked up on the next boot.
That is also why the map survives a refresh: keep the console open, re-export,
load again, and you have a quick edit loop.

## Live preview from the editor

Both kits can hand you the whole line ready to paste:

- Unity: **Copy JSON to clipboard for live preview** in the Map Kit window.
- Unreal: Tools ▸ Blox 4 Dead ▸ **Copy Live Preview Snippet**.

That copies a `window.B4D_CAMPAIGN_OVERRIDES = ...` assignment, which applies to
the current page only and is gone on refresh. For an edit loop that survives
reloading, `B4D_LOAD_MAP` is the one you want.

## Shipping a map for real

Local storage is for playtesting. To ship a map, paste its JSON into
`CAMPAIGN_DATA` in the game's HTML file, keyed by its `id`. It then builds for
everyone with no console step.

## When something is wrong

**The campaign looks unchanged.** Check `B4D_LIST_MAPS()` shows your map, and
that its `index` is 2, 3 or 4. Slots 0 and 1 ignore map data.

**The console says the map could not be built.** The game falls back to the
built in level rather than leaving you with nothing. The error names what broke.

**A prop is a plain grey box.** Its mesh could not be read. The console says
which asset and why. Collision is taken from the map data, so the box blocks
movement exactly as the real prop would.

**Everything is black.** Look for a bake warning about a material. See the
licensing and baking notes in the Unity kit's README.
