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

## Milestone D — Arrangement + shape the song (implemented)
Longer timeline with named non-overlapping sections, DrumClip migration,
clip duplicate/move/delete/resize (non-destructive), “turn this loop into a song”
beginner shape (Intro/Main/Variation/Outro) by repeating the 4-bar seed (no
time-stretch), deterministic variation helpers, whole-song + loop-region playback,
arrangement view, Sensei arrangement events, tests.

## Milestone E — Sound identity + instrument system (implemented on branch)
Stable Core instrument/preset IDs, engine Instrument interface, differentiated
built-ins (Warm Keys / Deep Bass / Bright Pluck / Studio Kit), snapshot routing
by instrument ID, beginner instrument picker, deterministic Sensei instrument
tips, tests. No plugins, no sample library, no full UI redesign.

## Milestone F — Native Studio UI/UX redesign (NOT started)
Major native Studio visual/UX redesign inspired by the clarity and workflow
quality of professional DAWs such as Ableton Live and Logic Pro, while retaining
Sensei Studio’s own identity. Do not copy proprietary design or assets.

Milestone F should address: tiny typography, excessive borders/grids, weak
hierarchy, cramped panels, generic engineering-tool appearance, poor clip
readability, and lack of a polished musical workspace.

Do not implement Milestone F during E.

## Later — Practice Views (NOT started)
Optional learning layouts inspired by familiar DAW workflows such as Ableton and
Logic, without copying proprietary assets or trying to replace those DAWs.
Practice Views are presentation/workflow layers over the same Core project model.
