# Sensei Studio

The next-generation music production environment that teaches producers while they create.

## Layout

- `prototype/` — browser UX reference (Sensei Studio 0.3). Not the shipping app.
- `libs/sensei_core/` — pure C++20 core (project model, transport, commands, Sensei analyzer).
- `libs/sensei_engine/` — audio device, MIDI scheduler, temporary SimpleSynth.
- `apps/sensei_studio/` — native JUCE application (piano roll + transport + Sensei panel).
- `docs/ARCHITECTURE.md` — layering, snapshot publication, dependency rules.
- `tests/` — Catch2 Core tests.

## Browser prototype

Open `prototype/index.html` in a desktop browser, then click **Start creating**.

## Native app

Requirements: CMake ≥ 3.24, a C++20 compiler, and platform audio/GUI libs.
JUCE **8.0.6** and Catch2 **v3.7.1** are fetched at configure time.

Run these commands from the `sensei-studio/` directory (the folder that contains
this README and the root `CMakeLists.txt`).

### Windows (Visual Studio 2022)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

App binary (typical path):

`build\apps\sensei_studio\sensei_studio_artefacts\Release\Sensei Studio.exe`

### macOS / Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Current milestones

- **A** — native shell, Core transport, SimpleSynth audition.
- **B** — piano roll, 4-bar loop playback, undo/redo, safe snapshots.
- **C** — multi-track guided first loop: chords, drums, root bass, Sensei flow (no AI).
- **D** — arrangement timeline, song sections, clip ops, turn-loop-into-song flow.
