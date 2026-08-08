# Architecture

Sensei Studio is a native Windows/macOS music-production learning environment.
The long-term stack is C++20 + JUCE + CMake. The browser prototype in `/prototype`
is a UX reference only and is not part of the shipping runtime.

## Layers

Dependency direction is strict and one-way:

1. **App Shell / UI** (`apps/sensei_studio`) — JUCE GUI, editors, Sensei panel presentation.
2. **Sensei Core** (`libs/sensei_core`) — project/transport/teaching model in pure C++20.
3. **Audio Engine** (`libs/sensei_engine`) — device I/O, graph, built-in voices.

Allowed dependencies:

- UI may depend on Core and Engine through explicit APIs.
- Engine may depend on Core only for realtime-safe transport/snapshot reads.
- Core must not depend on JUCE UI classes, Engine, networking, or AI.
- Engine must not depend on UI, Sensei teaching logic, networking, or AI.

Essential product functionality must work with AI disabled or absent. Any future AI
mentor is an optional adapter that consumes Core facts and proposes suggestions;
Core remains authoritative.

## Canonical musical data

The UI never owns the project. `sensei::core::Document` owns:

- `Project` / `Track` / `MidiClip` / `MidiNote` / `LoopRegion`
- `Transport`
- `CommandHistory` (undo/redo)
- `SnapshotPublisher`

Musical edits go through Core commands (`AddNote`, `DeleteNote`, `MoveNote`, `ResizeNote`).

## Realtime snapshot publication

Sequence data is published with a **fixed triple-slot** `SnapshotPublisher`:

- Three `SequenceSnapshot` objects live for the process lifetime (fixed capacity notes).
- The message thread writes into a slot that is not currently published, then atomically
  stores the published slot index.
- The audio thread only loads that index and reads the slot.
- **No `shared_ptr` snapshot ownership** crosses into the audio callback, so the audio
  thread never destroys heap snapshot memory.

Transport fields used by the audio thread are atomics. Musical timing advances inside
the audio callback via `Transport::advance`. UI timers may only repaint the playhead.

## Realtime safety

The audio callback must not:

- block (locks, sleeps, condition variables, synchronous I/O)
- allocate unpredictably (heap growth, logging strings, JIT work)
- touch the filesystem or network
- invoke AI / teaching side effects
- manipulate JUCE UI objects

## Sensei observations

`ProjectAnalyzer` produces deterministic factual observations from project state.
Create mode stays quiet: the panel updates for high-signal facts (empty clip, first
idea, notes outside loop) and when explicitly refreshed — not on every low-priority edit.
No LLM or network AI in Core.

## Milestone status

- **A** — native heartbeat (shell, transport, SimpleSynth audition).
- **B** — first musical loop (project model, piano roll, sequenced playback, undo/redo,
  deterministic Sensei observations).
