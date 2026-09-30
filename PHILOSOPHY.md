# Tetrodotoxin Philosophy

A language should keep the meaning that makes it useful. A source language,
build description and execution model can cooperate without translating every
fact into one universal graph representation.

TTX supplies the negotiated boundary. A provider exposes the capabilities and
policies it can support, and consumers continue through those policies when
asking further questions. Tetrodotoxin owns the concrete language contracts
built on that foundation.

Source owns authored bytes, lexical observations and source evidence. Execution
owns executable behavior. Library composes the language semantics that require
those building blocks. A source location is useful to an editor without becoming
part of every executable value, and execution should be usable without a parser.

Keep each fact with its real owner. A consumer asks that owner for the contract
it understands instead of maintaining a second interpretation of the same state.
Retention is an explicit contract rather than an assumption made when copying
an interface view. Provider code remains available while acquired interfaces
can still invoke it, including their release operations.

Incomplete source still has useful meaning. Preserve its original text, precise
locations and recoverable observations. Distinguish that evidence from permission
to produce an executable artifact.

Build a feature because a real consumer needs it. Old code can reveal useful
behavior and missed edge cases, but its inheritance, naming and build graph do
not define the next design. Independently built consumers provide evidence that
a boundary works beyond its native implementation.
