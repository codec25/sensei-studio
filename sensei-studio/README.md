# Sensei Studio

The next-generation music production environment that teaches producers while they create.

## Layout

- `prototype/` — browser UX reference (Sensei Studio 0.3). Not the shipping app.
- `libs/sensei_core/` — pure C++20 core (transport, later project/teaching model).
- `libs/sensei_engine/` — audio device + temporary SimpleSynth.
- `apps/sensei_studio/` — native JUCE application.
- `docs/ARCHITECTURE.md` — layering and dependency rules.
- `tests/` — Catch2 Core tests.

## Browser prototype

Open `prototype/index.html` in a desktop browser, then click **Start creating**.

## Native app (Milestone A)

Requirements: CMake ≥ 3.24, a C++20 compiler, and platform audio/GUI libs.
JUCE **8.0.6** and Catch2 **v3.7.1** are fetched at configure time.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run the app from the build tree (path varies by generator/OS), for example:

```bash
./build/apps/sensei_studio/sensei_studio_artefacts/Release/Sensei\ Studio
```

Milestone A provides: native shell, Core-owned transport (Play/Stop/BPM),
audio device init, SimpleSynth audition keyboard, and a static Sensei placeholder.
No AI, piano roll, plugins, or mixer yet.
