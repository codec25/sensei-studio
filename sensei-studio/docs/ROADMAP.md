# Roadmap

## Milestone A — Native heartbeat (implemented)
Native C++20/JUCE shell, Core-owned transport, audio device init, temporary
SimpleSynth, clickable audition keyboard, Catch2 Core transport tests.
Browser prototype preserved under `/prototype`.

## Milestone B — First musical loop (implemented)
Project model, piano roll, sequenced 4-bar loop playback, undo/redo foundation,
deterministic Sensei observations, safe snapshot publication, sample-accurate MIDI.

## Milestone C — First guided 4 bars (implemented)
Multi-track foundation (Chords/Bass/Drums/Melody), chord helper, drum grid,
root-note bass helper, temporary built-in sounds, guided first-loop Sensei flow,
compound undo/redo, learning events. No AI.

## Milestone D — Arrangement + shape the song (implemented on branch)
Longer timeline with named non-overlapping sections, DrumClip migration,
clip duplicate/move/delete/resize (non-destructive), “turn this loop into a song”
beginner shape (Intro/Main/Variation/Outro) by repeating the 4-bar seed (no
time-stretch), deterministic variation helpers, whole-song + loop-region playback,
arrangement view, Sensei arrangement events, tests.

Section overlap rule: named sections must not overlap; create/resize that would
overlap is rejected. Clip overlaps remain allowed.

## Later — Practice Views (NOT started)
Optional learning layouts inspired by familiar DAW workflows such as Ableton and
Logic, without copying proprietary assets or trying to replace those DAWs.
Practice Views are presentation/workflow layers over the same Core project model —
not separate project formats. Do not implement Practice Views in Milestone D.

## Milestone E
Not started.
