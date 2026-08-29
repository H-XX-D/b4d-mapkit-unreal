# Blox 4 Dead Map Kit for Unreal

Author campaign maps in the Unreal level editor and export them as JSON the
game loads directly. No engine code changes to add a map.

```
Unreal level  ──►  campaign JSON  ──►  playable map
```

The Unity edition of the same kit exports the identical format:
[b4d-mapkit-unity](https://github.com/H-XX-D/b4d-mapkit-unity).

## Install

Clone into your project's `Plugins` folder. The repository root is the plugin,
so it drops straight in:

```
git clone https://github.com/H-XX-D/b4d-mapkit-unreal.git Plugins/B4DMapKit
```

Reopen the project and let the editor build the module. Tested against Unreal
5.x. If the project has no C++ target yet, Unreal will offer to generate one.

## Use

Placeable actors appear under **Blox 4 Dead** in the Place Actors panel. Each is
Blueprintable, so you can derive a Blueprint to preset a theme for a set of maps.

| Actor | What it is |
| --- | --- |
| B4D Campaign | One per level. Holds the id, slot, theme and extraction. |
| B4D Zone | A room or corridor, sized with the normal scale widget. |
| B4D Objective | A chapter's device, with its nodes and cart destination. |
| B4D Gate | The checkpoint door that objective opens. |
| B4D Fuel Barrel | A barrel that goes up when shot. |
| B4D Drop Hazard | A heavy load on a cable that falls when shot. |
| B4D Prop | Set dressing. Pick a type, fill in its numbers. |

Then use **Tools ▸ Blox 4 Dead**:

- **Check Campaign Map** runs the game's own rules and reports what it finds in
  the Output Log.
- **Export Campaign JSON** checks first, then writes the file.
- **Copy Live Preview Snippet** puts one line on the clipboard. Paste it into
  the game's console and restart the campaign to play the map without a rebuild.

Everything is also exposed to Blueprint through `UB4DExporter`, so a project can
drive the export from its own editor utility widget instead of the menu.

## Coordinates

Unreal works in centimetres with Z up. The game works in metres on the X/Z
ground plane with Y up. The conversion lives in one place, `B4DSpace` in
`Source/B4DMapKit/Public/B4DMapKitTypes.h`:

| Unreal | Game |
| --- | --- |
| X / 100 | X |
| Y / 100 | Z |
| Z / 100 | Y (up) |

## The format

A map is one JSON document. `schema/b4d-campaign.schema.json` is the full
specification and `schema/examples/example_depot.json` is a small map that
passes every check, useful as a starting point.

```json
{
  "schema": 1,
  "id": "example_depot",
  "index": 2,
  "theme": "slaughterhouse",
  "zones": [
    { "name": "depot", "x": 0, "z": -12, "halfX": 30, "halfZ": 26,
      "floor": "tile", "roof": 9, "lampY": 8.4 }
  ],
  "objectives": [
    { "chapter": 1, "x": 0, "z": -12, "label": "RESTART THE DEPOT PUMPS",
      "verb": "PUMPS", "kind": "pumps", "type": "signal", "duration": 20 }
  ]
}
```

### Zones are the map

The walkable area is the union of the zones. Where two zones overlap by more
than 1.5 metres, that overlap is the doorway between them. Navigation,
spawning, objective placement and the interior architecture pass are all
derived from the zone list, so authoring a map is mostly drawing boxes and
naming them.

A zone that overlaps nothing else can never be entered. That is an error and
the exporter refuses to write the file.

### What gets checked before export

Errors, which block the export:

- a zone that overlaps no other zone
- two zones sharing a name, or a zone with no name, area or floor material
- two objectives claiming the same chapter
- an escort objective with no cart destination
- a solid prop with a zero sized collider
- a drop hazard whose cable anchor is at or below its load

Warnings, which do not block it:

- an objective outside every zone (the game relocates it to the nearest clear
  spot at load, so it plays, just not where you drew it)
- a hazard outside every zone (nothing relocates those, so it can never be
  shot or triggered)
- a breakers objective with no window, so its switches never have to line up
- a gate count that does not match the objective count

That last pair is worth taking seriously. Running these rules over an existing
hand-authored campaign turned up two fuel barrels and a falling carcass rack
sitting outside the walkable zones, left behind when the zones were moved and
the set dressing was not.

## Prop types

Set dressing is described by numbers rather than meshes, so the game builds it
procedurally and a map file stays small.

| Type | What it builds |
| --- | --- |
| `box`, `cylinder` | A single solid, optionally blocking |
| `grid` | A repeating grid of sub-props, e.g. pen rails |
| `chainLine` | An overhead rail with hanging chains and hooks |
| `carcassRows` | Rows of hanging carcasses on alternating offsets |
| `lightPole` | A pole with a light on top |
| `vat` | An open topped vessel with a surface disc |
| `pipeRun` | A straight run of stepped horizontal pipes |

Adding a type means adding a builder in the game, a row to the field table in
this plugin, and the name to the schema enum. No new class is needed: the
editor drives its fields off that table.

## Licence

MIT. See [LICENSE](LICENSE).
