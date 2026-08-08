SENSEI STUDIO 0.3 — FUNCTIONAL BROWSER PROTOTYPE (UX REFERENCE)

This folder is the preserved browser prototype. It is not the native app.
Native Sensei Studio lives under apps/sensei_studio (C++20/JUCE).

HOW TO OPEN
1. Open prototype/index.html in a desktop browser.
2. On the welcome screen, click Start creating.
3. Your browser will enable audio after that user click.

WHAT WORKS
- Real Web Audio synth sound
- Click keyboard notes to audition them
- Click the piano roll to add MIDI notes and hear them
- Right-click notes to delete them
- Real looping playback
- BPM changes playback speed
- Mute and Solo affect playback
- Drum track plays a simple synthesized kick + hat pattern
- Tiny built-in synth: waveform, brightness/filter, release
- Add MIDI tracks
- Local browser save/load
- Sensei recommendations are based on actual project state (note count, pitch variety, empty bass track)
- Create mode stays mostly quiet; Arrange/Mix/Finish offer different feedback

IMPORTANT
This is still a browser prototype. It is deliberately small and not yet a native DAW. The purpose is to validate the music-making and teaching interaction before moving to the long-term C++/JUCE implementation.

NEXT TARGET
- velocity editing
- note resizing/moving
- selectable loop length
- actual bass/drum editing
- simple chord helper
- guided "first 4 bars" lesson
- better Sensei A/B experiments
