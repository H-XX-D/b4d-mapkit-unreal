# Licensing

Short version: the kit is MIT and yours to use. What you bake with it is a
separate question, and the answer depends on where the art came from.

I am not a lawyer and this is not legal advice. It is a map of where the sharp
edges are so you know what to check.

## The kit itself

Everything in these repositories, the Unity package, the Unreal plugin, the
schema and the tools, is MIT, copyright H-XX-D. Use it commercially, modify it, ship it, no
attribution needed beyond keeping the licence file.

MIT covers **the code**. It says nothing about anything the code produces from
your inputs.

## What you bake

A `.glb` produced by the baker contains the meshes and textures you fed it. The
kit's licence gives you no rights to that content. Whatever governed the source
art still governs the baked copy.

Three cases:

**Your own art.** Nothing to think about. Bake and ship.

**Openly licensed art**, CC0, CC-BY, and similar. Usually fine. CC-BY needs
attribution somewhere a player can find, and a credits screen counts. Check
whether the licence has a share-alike clause, because that can reach your whole
work.

**Unity Asset Store art.** This is the one to be careful about, and the reason
the kit has a reference-only path.

## The Asset Store problem

The standard Unity Asset Store EULA lets you use an asset in an interactive
product. It also restricts distributing the asset in a form that lets someone
extract the original, because that would let them use it without buying it.

A web game is close to the worst case for that clause. A `.glb` served to a
browser is a plain file sitting in the network tab. Anyone can save it and open
it in Blender. Whether that counts as prohibited redistribution depends on the
specific asset's licence and on the publisher, and packs vary: some explicitly
allow web builds, some explicitly forbid them, most say nothing and leave you
guessing.

So before baking a purchased asset into a shipped map:

1. Read that pack's licence, not the general Asset Store terms. Publishers
   attach their own.
2. Look for anything about web, WebGL, browser, or extraction.
3. If it is silent, ask the publisher. Most answer.
4. If you cannot get an answer, do not ship it.

## Shipping inside a Discord Activity

This matters, and it moves the answer, so it is worth being precise about what
it does and does not change.

**What it changes.** A Discord Activity is served to the Discord client from
your own server. Players open a game, not a web page: there is no address bar,
no view source, and nothing in a search index. Casual exposure is close to zero,
which is a real difference from a public web build sitting on an open URL.

**What it does not change.** It is still a web page underneath. The Discord
desktop client can open developer tools, and the activity is reachable over
HTTPS. Someone determined can still pull what the page loaded. Being behind
Discord lowers practical exposure; it does not create a licence exception.

**Why that is usually fine anyway.** The Asset Store EULA permits incorporating
an asset into an interactive product. The clause people worry about is aimed at
distributing the asset *as an asset*, in a form someone can take and use in
their own work. A mesh baked to glTF, stripped of its names, optimised for one
game and embedded in that game's page is on the incorporated-into-a-product side
of that line by any ordinary reading. This is the same posture as every
commercial WebGL game shipping bought art.

The residual risk is narrow and specific: **packs whose own terms explicitly
prohibit web or browser builds.** Some say so outright. That is the thing worth
checking, rather than the general question of whether a browser can technically
save a file.

### What the kit does to help

| Measure | Effect |
| --- | --- |
| Bake to glTF | Ships a game-ready derivative, not the source FBX, and drops everything the game does not use. |
| Inline as base64 | On by default. The mesh lives inside the page rather than as a `.glb` sitting in the network tab, so there is no file to right click and save. |
| Strip names | On by default. Mesh, node, material and generator names are removed. Names are the part that identifies which pack a mesh came from. |
| One base colour map | No normal, roughness or emissive maps are carried, so the full material set never leaves Unity. |

Turn stripping off while you are debugging a bake; leave it on for anything you
ship.

Your own side of it is server shaped and outside this kit: serving the activity
only to authenticated Discord sessions, and not leaving the build on a public
URL, are both worth doing and both live in your backend.

### What I would actually do

1. Ship it. For the overwhelming majority of packs this is licensed use.
2. Before baking a pack, search its licence page for "web", "WebGL" or
   "browser". If it forbids them, use reference scenery for that pack instead.
3. Keep inline and strip names on, which is the default.
4. If a pack is expensive, central to the look, or the terms read oddly, ask the
   publisher. Most answer, and a one line yes in your inbox is worth having.

## The safe path

**Reference scenery exists for exactly this.** Mark art with
`B4D Reference Scenery` and it never leaves Unity. You still get to build the
level against real geometry: fit zones to a building, take colliders from
renderer bounds, see what the space actually feels like. Only boxes and gameplay
objects travel to the game.

That gets you the whole benefit of designing against real art with none of the
redistribution question, because nothing is redistributed.

Use `B4D Model Prop` and the baker for art you own or that is openly licensed.

## Third party code in the kit

None. The Unity package and the Unreal plugin have no dependencies beyond the
engine itself. The JSON reader and writer, the glTF baker and the glTF reader
in the game are all written for this project, which is partly why they are
deliberately small.

The game loads three.js from a CDN, which is MIT. That belongs to the game, not
to these plugins.

## The reference stubs

`Tools~/compile-check` contains stub declarations of Unity API signatures so the
package can be type checked without Unity installed. They are written from the
public API documentation and contain no Unity code. They are never compiled into
a build, and Unity ignores the folder because its name ends in `~`.

If you would rather not carry them, delete the folder. Nothing else depends on
it.

## Summary

| What | Licence | Watch out for |
| --- | --- | --- |
| This kit's code | MIT | Nothing |
| Maps you author | Yours | Nothing |
| Your own art, baked | Yours | Nothing |
| CC0 art, baked | CC0 | Nothing |
| CC-BY art, baked | CC-BY | Credit it, check for share-alike |
| Asset Store art, baked into a Discord Activity | That pack's EULA | Usually licensed use. Check only for an explicit web or WebGL prohibition. |
| Asset Store art, reference only | That pack's EULA | Nothing, it never leaves Unity |
