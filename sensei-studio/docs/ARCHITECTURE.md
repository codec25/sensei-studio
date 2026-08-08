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
- Engine may depend on Core only for realtime-safe transport/state reads.
- Core must not depend on JUCE UI classes, Engine, networking, or AI.
- Engine must not depend on UI, Sensei teaching logic, networking, or AI.

Essential product functionality must work with AI disabled or absent. Any future AI
mentor is an optional adapter that consumes Core facts and proposes suggestions;
Core remains authoritative.

## Realtime safety

The audio callback must not:

- block (locks, sleeps, condition variables, synchronous I/O)
- allocate unpredictably (heap growth, logging strings, JIT work)
- touch the filesystem or network
- invoke AI / teaching side effects
- manipulate JUCE UI objects

Transport fields used by the audio thread are stored in atomics / POD snapshots.
UI reads those fields for display; UI commands mutate Core on the message thread
via explicit APIs.

## Milestone A scope

Native shell, Core-owned transport (play/stop/BPM/position), audio device init,
temporary `SimpleSynth`, clickable note audition, static Sensei placeholder,
Catch2 Core transport tests.

Out of scope until explicitly authorized: piano roll, project model, sequencing,
undo/redo, plugin hosting, mixer, cloud/auth, LLM integration, Electron.
