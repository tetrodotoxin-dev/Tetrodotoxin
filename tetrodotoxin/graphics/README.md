# Graphics

Graphics turns completed Scene Objects into immutable frame draws without
making Scene understand Vulkan or making the backend understand authored TTX
Objects. It is a runtime and Terminal boundary, not another Dialect or a second
scene graph.

## Drawable interfaces

The ordinary `Perimortem.Graphics` Package publishes a real Library Interface:

```ttx
public DrawableUI : interface {
  public state material  : Implementation[Pipeline];
  public state transform : Transform2D;
  public state visible   : Bool = true;
  public state z_index   : S64;
}
```

`Implementation[Pipeline]` is explicit Object-backed erasure for one exact
higher-order Interface relation. It retains the concrete Shader Material and
its immutable ABI projection. It does not copy declarations or rely on a
structural naming convention.

A concrete Type names the Interface it implements:

```ttx
public Sprite : implementation DrawableUI {
  public state texture : Texture2D;
  public state size    : Size2D;
}
```

The implementation materializes the Interface state once, before Sprite's own
Fields. Sprite therefore does not redeclare `material`, `transform`, `visible`,
or `z_index`, while ordinary access continues to select them on the Sprite.
The generated ABI layout preserves that exact prefix for Interface projection.

## Sprite owns the quad draw

`Image` is the shared decoded CPU identity. `Sampler2D` is independent
addressing and filtering policy, and inline `Texture2D` pairs the two without
another allocation. A Sprite contributes that sampled value, size, Material,
and its fixed draw policy: unit-quad geometry, triangle-list topology, alpha
blending, and six vertices.

Those fixed facts do not belong to the Pipeline or Shader. A frame Batch carries
them beside the selected Program, resources, parameter bytes, transform, size,
and z index. Vulkan caches a native realization by Program plus fixed draw
state, so another drawable can select another compatible state without
hard-coding that policy into the Shader generator.

The concrete Material remains the owner of authored uniforms and additional
resources. Frame collection copies its current parameter bytes and follows the
generated resource projection in declaration order. For example, Scene
Lifetime's Blend Material owns its noise Texture2D while Sprite still supplies
the primary texture required by the textured-quad Pipeline.

## Passes

`PassUI` gathers Objects that implement `DrawableUI`, rejects invalid cycles,
freezes their current draw values, and sorts accepted Batches in ascending z
order. Authored traversal order breaks equal-z ties. Visibility removes a
complete hosted subtree, and transforms compose through that traversal.

A 3D pass is a separate capability because depth-tested geometry has a
different collection and ordering contract. It can share Material and Pipeline
machinery without making `DrawableUI` or Sprite imply a camera or depth buffer.
Pass ownership also leaves room for a Camera or Viewport to select which passes
participate in a frame without moving those concerns into Shader.

## Stable frame and backend boundary

Each Batch is an immutable backend-neutral draw transaction. Sampled resources
retain their Image Objects and copy their Sampler2D values for the frame;
parameters and host inputs are copied. Later Scene mutations therefore affect
the next frame, never one already being presented.

The generated application product maps exact Types that satisfy `DrawableUI`
to their runtime draw providers. Graphics follows that exact semantic proof;
it does not match a Type by a list of familiar Field spellings. Vulkan then
consumes ordered Batches and generated Program descriptions, realizes textures
and pipelines, records commands, synchronizes the device, and presents.

See [Scene](../scene/README.md) for application state,
[Pipeline](../render/README.md) for material contracts,
[Shader](../shader/README.md) for GPU implementations, and the
[standard Packages](../../packages/ttx/README.md) for the authored surface.
