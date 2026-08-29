# Licensing

Short version: the kit is MIT and yours to use. What you bake with it is a
separate question, and the answer depends on where the art came from.

I am not a lawyer and this is not legal advice. It is a map of where the sharp
edges are so you know what to check.

## The kit itself

Everything in these repositories, the Unity package, the Unreal plugin, the
schema and the tools, is MIT. Use it commercially, modify it, ship it, no
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
| Asset Store art, baked | That pack's EULA | Extraction and web clauses. Check before shipping. |
| Asset Store art, reference only | That pack's EULA | Nothing, it never leaves Unity |
