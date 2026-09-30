# Source

Source supplies token streams, locations and parsing contracts. A Dialect exposes
`Parse` when it can interpret a stream. It owns the meaning it produces and the
storage that keeps that meaning alive.

## Import and Stream

The Source module exposes TTX `Import` through `ttx_query`. Its input contains a
path label and an Abstract whose data observation supplies source bytes. The path
never requests filesystem access and is copied. Import first negotiates `Borrow`
on the byte owner. A successful acquisition supplies the data view and remains
retained until the resulting Stream is released. Source does not copy those bytes.

Unknown or Rejected from binding or acquisition selects a copy of the original
subject's current data. Import then runs the TTX Tokenizer and lends an Abstract
supporting `Stream` and `Borrow` during the receiver callback. A byte owner can
supply a different acquired answer, so Source reads that answer only after
acquisition. An acquired source snapshot must keep its bytes unchanged until
release. The caller keeps that byte provider's code loaded for the same lifetime.

Representation mismatches, missing input, nonempty null views and values outside
the compact token profile return Rejected without calling the receiver. Native
callers supply accessible storage matching their declared representation.
Unknown lexical spans remain in accepted streams so editors can observe and
format incomplete source. Import does not validate a program's grammar.

`Stream` exposes source input, a lexical contract identity and a described token
buffer. The descriptor describes one record, and the buffer gives its byte size
and token count. The native reader admits the compact representation once, then
uses indexed reads without calling the provider for each token. A foreign owner
can supply the same buffer from its own storage without a C++ Arena.

Lexical identity and physical representation answer different questions. Equal
record layouts do not make two vocabularies equivalent. The published TTX lexical
identity names the classifications in `Code` and the spellings in `Lexicon`,
including keywords and compound operators. Formatter checks that identity before
formatting and returns a typed failure for unsupported input.

## Cursor and Parse

Cursor owns a position within a borrowed Stream and a bounded token range. It
provides lookahead, matching and consumption. It owns no allocator, diagnostic
collection or association index. Recovery code chooses its stop set and whether
the enclosing parser should see that boundary.

A consumer binds the selected Dialect's `Parse` capability and supplies a Cursor,
a Diagnostics subject and a receiver. The parser checks the lexical contract and
token representation, and negotiates the supplied diagnostic policy, before
consuming input. A refusal at admission leaves the cursor unchanged.

After admission, consumption and published diagnostics are effects. A failing
parse does not roll them back. Satisfied invokes the receiver exactly once with a
borrowed Abstract projection. That projection can carry partial meaning and does
not establish that a program is complete or executable. Unknown and Rejected
never invoke the receiver.

The canonical C cursor record carries the caller's stream, bounds and position.
Nested parsers update that position, preserving the stream and bounds. Reaching
the exclusive end yields a zero-length Terminal without consuming the enclosing
token. Out-of-range lookahead returns an invalid Token.

## Diagnostics and Errors

Diagnostics receives source bytes, an Anchor, a message and an optional hint.
Those views are borrowed until the report call returns. A retaining sink copies
the evidence it needs. The reporting capability also exposes a count so an outer
parser can avoid duplicating a more precise nested error.

Errors supplies the native collector and renderer. Its reporting helpers accept
an explicit Cursor for source context. It copies messages, hints and source
snapshots into its own Arena, keeping diagnostics usable after parser teardown.
One Errors collection represents one snapshot per exact path. Separate source
versions require separate collections because a path is a label, not a version
identity. Associations stays with the source or result owner maintaining that
index, independently of cursor traversal.

## Compact token profile

The eight-byte record retains a 16-bit byte offset, line and column, plus an
8-bit size and Code. Lines and columns start at one. A token spans at most 255
bytes. Source offsets, lines and columns must fit before publication. The
Tokenizer rejects a stream that would truncate any field.

The tokenizer retains the concrete TTX lexical rules. LF advances the line,
while CR is whitespace. Every complete stream includes one zero-length Terminal
at the source byte count. Token indices are independent of byte coordinates.
The buffer descriptor makes its physical layout explicit to consumers.

## Lifetime

Stream, Parse and Diagnostics views acquire no lifetime when copied or bound.
A receiver retaining a parse result acquires TTX `Borrow` during the callback.
The returned `Borrowed` answer has one explicit release obligation. A retained
result must acquire or copy every source observation it continues to use.

The Source module retains bytes and tokens together. Binding another interface
from an acquired answer preserves its Borrowed policy. Release every acquisition
before unloading its supplying code. Native observations and releases remain on
the owning worker.

The C records and callable tables define the public boundary. The C++ facades
use those same contracts without recovering a provider's native object layout.
