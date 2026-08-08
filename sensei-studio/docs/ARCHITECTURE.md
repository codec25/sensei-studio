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

Essential product functionality must work with AI disabled or absent.

## Canonical musical data

The UI never owns the project. `sensei::core::Document` owns:

- `Project` / `Track` / `MidiClip` / `MidiNote` / `LoopRegion`
- `Transport`
- `CommandHistory` (undo/redo)
- `SnapshotPublisher`

Musical edits go through Core commands (`AddNote`, `DeleteNote`, `MoveNote`, `ResizeNote`).
The piano roll may keep a **transient preview** for drag/resize/create visuals, but
canonical `Project` note fields change only when a command is executed (typically on mouse-up).

## Realtime snapshot publication

`SnapshotPublisher` uses an explicit front/back + audio-local copy strategy
(no `shared_ptr`, no audio-thread heap free):

1. Message thread writes into private `back_` (audio never reads `back_`).
2. `publish()` copies `back_` → `front_` under a mutex (message thread may block here).
3. Audio thread `beginRead()` uses `mutex_.try_lock()`:
   - **Acquired:** copies fixed-size POD `front_` → `audioLocal_`, unlocks.
   - **Busy:** keeps the previous `audioLocal_` (last coherent snapshot).
4. The audio callback reads only `audioLocal_` for schedule+render.

### Synchronization invariant

- Audio never blocks on the mutex (`try_lock` only), never heap-allocates/deallocates.
- Audio never reads memory the message thread is mutating.
- `audioLocal_` is written only on the audio thread; lifetime is the publisher object.
- A successful copy is a coherent snapshot; a skipped copy is at most one publish late.
- `publishedIndex`-only triple-slot schemes are insufficient without a reader-lifetime
  claim; this design makes ownership explicit via the audio-local buffer.

## Sample-accurate MIDI scheduling

`MidiScheduler` collects note-on/note-off events with sample offsets inside the
current audio block (including loop-boundary split ranges), sorts them with an
allocation-free insertion sort (note-offs before note-ons at the same offset),
and renders `SimpleSynth` in segments between events. Musical timing advances
via `Transport::advance` on the audio timeline. UI timers may only repaint the playhead.

## Realtime safety

The audio callback must not:

- block (locks, sleeps, condition variables wait, synchronous I/O)
- allocate unpredictably (heap growth, logging strings, JIT work)
- touch the filesystem or network
- invoke AI / teaching side effects
- manipulate JUCE UI objects

## Sensei observations

`ProjectAnalyzer` produces deterministic factual observations from project state.
Create mode stays quiet except high-signal facts. No LLM or network AI in Core.

## Milestone status

- **A** — native heartbeat (shell, transport, SimpleSynth audition).
- **B** — first musical loop (project model, piano roll, sequenced playback, undo/redo,
  deterministic Sensei observations), with safe snapshot publication and sample-accurate scheduling.
