# Pipeline

Pipeline is the target-neutral interface between a draw provider and a Shader.
It names the resources, host inputs, and ordered Stage layouts that both sides
must satisfy. It does not render, own executable code, or choose Vulkan state.

One Pipeline source owns one interface. There is no nested contract Type:

```ttx
// One textured quad interface.
dialect : Pipeline;

public Math : alias = package(.name = "Perimortem.Math", .version = "1.0");
private Image : alias = source("../image.ttx")::Image;

public image : resource read Image;
public inputs : push Inputs;

public vertex : stage [
  .position : Math::Vec2D,
  .texture_uv : Math::Vec2D,
] -> [
  .position : Math::Vec4D,
  .texture_uv : Math::Vec2D,
];

public fragment : stage [
  .texture_uv : Math::Vec2D,
] -> [
  .color : Math::Vec4D,
];

public Inputs : struct {
  public transform_x : Math::Vec4D;
  public transform_y : Math::Vec4D;
}
```

Canonical grammar reference: [Render.g4](grammar/Render.g4). The implementation
directory retains its historical `render` spelling, while the authored Dialect
and public concept are Pipeline.

## Ordered interface meaning

Pipeline declarations are semantic facts rather than a second backend IR:

* `resource read T` requires one readable resource with the exact sampling Type
  `T`. Other access spellings can describe write or read/write resources when a
  target supports them.
* `push T` names host-supplied values available to Stage code.
* `stage` names one required entry and its complete input and output Layouts.
* `struct` groups related Pipeline values without manufacturing a Library Type.

Stage Layouts are ordered. Non-builtin entries receive locations in that order,
so authors do not repeat location numbers. The exact names `position` and
`vertex_index` select their standard GPU builtin roles in the output and input
domains respectively. Host fields likewise retain their authored names through
generation; a draw provider and generated target glue agree on those names
without an authored `@host` mapping.

Resource declaration order supplies stable descriptor order. A Shader can
implement a Pipeline resource directly or pair a CPU-visible material carrier
with the required GPU sampling Type. Sets, slots, storage classes, offsets, and
SPIR-V decorations are generated target facts, not authored Pipeline facts.

## What Pipeline deliberately does not own

Topology, blending, geometry, vertex count, cameras, depth behavior, and draw
ordering belong to the Object or pass that contributes a draw. The same Shader
can therefore be attached to compatible draw providers without its interface
silently selecting one engine policy. A frame carries the selected fixed state
beside the Program, and a backend realizes the corresponding native pipeline.

Pipeline also owns no Shader expression or runtime material state. It remains a
declarative interface that editors, Archives, Shader validation, and different
GPU targets can inspect before a representation is selected.

## Shader relationship

A Shader selects one Pipeline source explicitly:

```ttx
implements source("../pipelines/textured_2d.ttx");
```

The expression may instead end at a Package export and can use any number of
`::` segments. The selected Pipeline Monograph is the requirement. Shader
retains that exact edge, authors explicit Stage bodies, and proves each body
signature against the matching ordered Pipeline Stage.

Pipeline knows nothing about Shader or Vulkan. Shader builds executable meaning
against the interface, while a Vulkan or another backend later generates CPU
and GPU glue from the completed graph.

## Persistence

Pipeline members can be stored in a Package product and reconstructed without
their source file. The complete graph retains public and private declarations
but no generated locations, descriptor coordinates, SPIR-V, native handles, or
frame state.

See [Shader](../shader/README.md) for implementations and
[Graphics](../graphics/README.md) for draw and pass ownership.
