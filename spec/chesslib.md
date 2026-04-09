# chesslib spec

## Objectives

1. **Standalone production-ready library.** Correct, well-tested, clean C++20 API.
   No engine code (eval, search, random play). Suitable for public release.

2. **Foundation for a chess database application.** Provides everything the database
   layer needs for chess rules and move handling — nothing more.

---

## Scope

### In scope
- 0x88 mailbox board representation
- Legal move generation (pseudo-legal + legality filter)
- FEN import/export
- SAN import/export
- Zobrist hashing (full recompute + incremental building blocks)
- Compact 16-bit move encoding for storage
- Game state: side to move, castling rights, en-passant, halfmove clock, fullmove number

### Out of scope
- Search / evaluation
- UCI protocol (may live in a separate utility, not the core library)
- Opening books, tablebase probing
- Game (PGN) parsing — belongs in the database layer

---

## Design principles

- **Follow CPW convention** ([chessprogramming.org](https://www.chessprogramming.org)) where applicable.
  Exception: CPW often presents educational or illustrative solutions; prefer the performant
  alternative when one exists and CPW itself acknowledges the trade-off.

---

## API requirements

### Board
- `board` constructed from FEN string or default position
- Private piece/color storage; public read-only accessors (`piece_at`, `color_at`, `operator[]`)
- `is_attacked`, `is_king_in_check`, `is_checkmate`, `is_stalemate`
- `move_maker`: `make()`, `undo()`, `check()` — `do_move`/`swap` remain private

### Move generation
- `move_generator::moves()` — pseudo-legal, const-correct
- `legal_moves(board&)` — filters pseudo-legal list via `move_maker::check()`

### SAN
- `parse_san(board const&, string_view) -> tl::expected<move, error>` — needs the board for disambiguation
- `to_san(board const&, move) -> string`

### Zobrist
- Full hash: `zobrist::hasher{}(board)` — always available
- Incremental building blocks: `hasher::piece()`, `hasher::enpassant_file()`, `hasher::side_to_move()` — for callers that maintain a running hash (e.g. the database import layer)
- Convention: white-to-move is the baseline; XOR `side_to_move()` only when black to move

### Move encoding
- In-memory representation: `struct move` (21-bit bit-field, stack-friendly)
- Storage representation: `u16 encode(move)`, `move decode(u16)` — for compact on-disk storage in the database

  16-bit layout (CPW convention):

  | Bits  | Field | Notes |
  |-------|-------|-------|
  | 15–10 | from  | 0–63, remapped from 0x88 |
  | 9–4   | to    | 0–63, remapped from 0x88 |
  | 3–0   | flags | see table below |

  | Flags | Meaning | | Flags | Meaning |
  |-------|---------|-|-------|---------|
  | 0000  | quiet              | | 1000  | knight promotion |
  | 0001  | double pawn push   | | 1001  | bishop promotion |
  | 0010  | kingside castle    | | 1010  | rook promotion |
  | 0011  | queenside castle   | | 1011  | queen promotion |
  | 0100  | capture            | | 1100  | knight promotion + capture |
  | 0101  | en passant capture | | 1101  | bishop promotion + capture |
  | 0110  | (reserved)         | | 1110  | rook promotion + capture |
  | 0111  | (reserved)         | | 1111  | queen promotion + capture |

  Decode helpers: `flags & 0x8` → promotion, `flags & 0x4` → capture, `flags & 0x3` → promo piece (0=N, 1=B, 2=R, 3=Q).

---

## Open tasks

*No open tasks.*

---

## Decided / closed

| Item | Decision |
|------|----------|
| `ply` → `halfmove_clock`, `count` → `fullmove_number` | Renamed to follow CPW convention. `halfmove_clock` stays `u8` (50-move rule, max 100). `fullmove_number` is `u16` (sufficient for any practical game length) |
| `pieces_`/`colors_` encapsulation | Done — private fields, public accessors, friends declared |
| `do_move`/`swap` visibility | Done — private |
| `scope_exit` utility | Done — `include/chesslib/core/scope_exit.hpp` |
| Zobrist side-to-move convention | Done — XOR only when `black_to_move()` |
| Incremental Zobrist API | Done — static helpers on `zobrist::hasher` provide the building blocks |
| Compact 16-bit move encode/decode | Done — `chesslib::codec::encode(move) -> u16` / `decode(u16) -> move` in `board/move_codec.hpp` |
| `parse_san` / `to_san` | Done — `chesslib::san::from_string(board&, string_view)` / `san::to_string(board&, move)` in `util/san.hpp` |
| `move_generator` const correctness | `board& b` with `moves() const` is correct — `const` applies to `move_generator`, not the board; scope_guard restores the board, no change needed |
