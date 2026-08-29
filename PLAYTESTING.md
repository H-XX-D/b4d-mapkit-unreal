# Playtesting a map you built

You are running the game as a downloaded HTML file, opened straight from disk.
No server, no build step. This is how you get a map you just edited into it.

## The short version

1. Export the map from Unity.
2. Open the game's HTML file in a browser.
3. **Drag the exported `.json` onto the game window.**

That is the whole loop. It reloads itself and the map is in.

Nothing is uploaded anywhere. The map is stored in that browser and stays there
until you clear it, so a refresh keeps it.

---

## In full

### 1. Export

**Window ▸ Blox 4 Dead ▸ Map Kit ▸ Export JSON.** The export refuses to write a
file if the map has errors, so a file that comes out will load.

### 2. Pick a slot

The `index` on the campaign root decides which campaign your map replaces.

| index | Campaign | Can you replace it |
| --- | --- | --- |
| 0 | NO MERCY, rail yard | no, still hand built |
| 1 | DEPARTURE, airport | no, still hand built |
| 2 | BLOOD MEAL, slaughterhouse | yes |
| 3 | DEAD TIDE, harbor | yes |
| 4 | BLACKOUT DETOUR, crosstown | yes |

Use **2**, **3** or **4**. The menu still shows the original campaign's name, so
a map in slot 2 is listed as BLOOD MEAL.

### 3. Drop it on the game

Open the HTML file in a browser and drag the `.json` anywhere onto the window.
An overlay confirms it, the map is checked, and the page reloads with it in.

If the map has errors the overlay says so and nothing is stored. The reasons are
in the browser console.

### 4. Play it

Start the game and pick the campaign for the slot you used.

---

## Editing loop

Leave the game open in one window and Unity in the other.

1. Change something in the scene.
2. Export over the same file.
3. Drag it on again.

The new version replaces the old one for that slot.

---

## Console commands

Drag and drop is easier, but these are there when you want them. Open the
console with:

- **Safari**: Develop ▸ Show JavaScript Console. Turn the Develop menu on first
  in Settings ▸ Advanced.
- **Chrome, Edge**: View ▸ Developer ▸ JavaScript Console.
- **Firefox**: Tools ▸ Browser Tools ▸ Web Console.

| Command | What it does |
| --- | --- |
| `B4D_LIST_MAPS()` | Which maps are loaded, and which slot each holds. |
| `B4D_CLEAR_MAPS()` | Removes them all and puts the built in levels back. |
| `B4D_LOAD_MAP(map)` | Loads a map from an object or a JSON string. |

`B4D_LOAD_MAP` takes either a pasted object or the text, so this works if you
would rather paste than drop:

```js
B4D_LOAD_MAP(`  paste the json between these backticks  `)
```

---

## Why it reloads

The world is built once when the page starts, so anything handed to it after
that is too late for that run. The map is stored first and picked up on the next
boot. That is also why it survives a refresh.

---

## Shipping a map properly

Storing in the browser is for playtesting. To make a map part of the game for
everyone, paste its JSON into `CAMPAIGN_DATA` in the game's HTML file, keyed by
its `id`. It then builds with no drop step.

---

## When something is wrong

**Nothing changed.** Run `B4D_LIST_MAPS()`. If your map is not listed, the drop
did not take. If it is listed but the level looks the same, check its `index` is
2, 3 or 4, since slots 0 and 1 ignore map data.

**The overlay said the map has errors.** The console lists them. The Map Kit
window in Unity reports the same things, with a Select button per problem.

**The console says a map could not be built.** The game falls back to the built
in level rather than dropping you into nothing. The error names what broke.

**A prop is a plain grey box.** Its mesh could not be read, and the console says
which and why. Collision comes from the map data either way, so the box blocks
movement exactly as the real prop would.

**Dropping does nothing at all.** Make sure you are dropping onto the game
window itself and that the file ends in `.json`. Some browsers open a dropped
file in a new tab if the page has not finished loading, so give it a moment
first.
