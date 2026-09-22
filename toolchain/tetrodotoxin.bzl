# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Hermetic source-graph compilation for Tetrodotoxin."""

def _ttx_source_impl(ctx):
    products = ctx.actions.declare_directory(ctx.label.name + ".products")
    dependency_products = depset(transitive = [
        dependency[DefaultInfo].files
        for dependency in ctx.attr.deps
    ])
    arguments = [
        ctx.executable._compiler.path,
        ctx.file.src.path,
        products.path,
        "1" if ctx.attr.generate_cxx else "0",
    ] + [product_tree.path for product_tree in dependency_products.to_list()]
    ctx.actions.run_shell(
        inputs = depset(
            direct = [ctx.file.src] + ctx.files.source_tree,
            transitive = [dependency_products],
        ),
        outputs = [products],
        tools = [ctx.executable._compiler],
        arguments = arguments,
        command = """
set -eu
compiler="$1"
source="$2"
products="$3"
generate_cxx="$4"
shift 4
mkdir -p "$products"
for dependency in "$@"; do
  while IFS= read -r -d '' product; do
    relative="${product#"$dependency"/}"
    target="$products/$relative"
    mkdir -p "$(dirname "$target")"
    if [ -e "$target" ]; then
      cmp -s "$product" "$target" || {
        echo "Conflicting staged Package product: $relative" >&2
        exit 1
      }
    else
      cp "$product" "$target"
    fi
  done < <(find "$dependency" -type f -print0)
done
set -- "$source" "-package_repository=$products" "-terminal_repository=$products"
if [ "$generate_cxx" = 1 ]; then
  set -- "$@" -generate_cxx
fi
"$compiler" "$@"
""",
        mnemonic = "TtxSourceCompile",
        progress_message = "Compiling TTX source graph %s" % ctx.label,
    )
    return [DefaultInfo(files = depset([products]))]

_ttx_source = rule(
    implementation = _ttx_source_impl,
    attrs = {
        "src": attr.label(
            mandatory = True,
            allow_single_file = [".ttx"],
            doc = "The one source passed to Puffer.",
        ),
        "deps": attr.label_list(
            doc = "Complete Package product trees staged into a private repository.",
        ),
        "generate_cxx": attr.bool(
            default = False,
            doc = "Requests the graph's optional canonical C++ products.",
        ),
        "source_tree": attr.label_list(allow_files = True),
        "_compiler": attr.label(
            default = "//puffer:puffer",
            executable = True,
            cfg = "exec",
        ),
    },
    doc = "Compiles one TTX source through Puffer and returns its product tree.",
)

def ttx_source(name, src, deps = [], generate_cxx = False, **kwargs):
    """Compiles one source graph without exposing build-tool artifacts to TTX."""
    if type(src) != "string":
        fail("ttx_source src must be one package-relative path")
    segments = src.split("/")
    root = "/".join(segments[:-1])
    prefix = root + "/" if root else ""
    source_tree = native.glob(
        [prefix + "**/*"],
        allow_empty = True,
        exclude = [src],
    )
    _ttx_source(
        name = name,
        src = src,
        deps = deps,
        generate_cxx = generate_cxx,
        source_tree = source_tree,
        **kwargs
    )
