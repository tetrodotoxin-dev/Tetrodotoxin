# Terminal producers

Once a Tetrodotoxin Workspace understands the complete program, Terminals turn
that meaning into products which can leave the live graph. Each producer owns
one destination and can therefore stay focused on the facts its next consumer
actually needs.

The native path begins with the [ABI Terminal](abi/README.md). It projects the
completed graph into the common C representation used for symbols, carriers,
calling boundaries, and cross language headers. This gives every native
producer one agreement without making a particular instruction engine the
source of representation truth.

The [LLVM Terminal](llvm/README.md) consumes that ABI and lowers Library
execution into LLVM modules and CPU objects. The Vulkan Terminal begins from
the Library execution graph hosted by Shader, follows its exact Pipeline contract
and Bridge relationships, and derives both the [SPIR V module](spirv/README.md)
and matching CPU pipeline description. Graphics compiles hosted Scene Fields
into compact access behavior. Application composition connects those products
to the App policy and selected runtime.

An application target pairs each configured drawable Type with its native draw
provider. That target configuration follows the successful `DrawableUI`
Interface proof without copying Fields. Child traversal stays an independent
runtime capability, while each draw supplies its own backend-neutral fixed
state beside the generated Program.

These producers share semantic identities only while the Workspace is alive.
The CPU object, embedded SPIR-V module, Scene access function, Vulkan pipeline
description, and generated process entry are sibling outputs after that handoff.

```text
Dialect and Workspace meaning
       /                 \
      v                   v
Terminal::Abi       Terminal::Vulkan
      |           GPU module and CPU glue
      v                   |
Terminal::Llvm            |
  CPU objects             |
       \                 /
        v               v
       Terminal::Application
          native entry
```

Archive writers, formatters, and Linker remain with their natural owners. They
are Terminal producers too, but moving every output beneath one directory would
hide the domain that defines its product.
