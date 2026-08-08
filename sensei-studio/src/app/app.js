const STORAGE_KEY='senseiFunctional03';
const ROW_H=22, STEP_W=40, STEPS=32; // 8 bars of eighth notes
const NOTE_NAMES=['C5','B4','A#4','A4','G#4','G4','F#4','F4','E4','D#4','D4','C#4'];
const MIDI_BY_ROW=[72,71,70,69,68,67,66,65,64,63,62,61];
const state={
  goal:'Create my first song', projectName:'My First Sensei Project', mode:'CREATE', playing:false,bpm:94,started:false,
  selectedTrack:0, positionStep:0, synth:{wave:'sawtooth',filter:1800,release:.25},
  tracks:[
    {id:'piano',name:'Sensei Synth',type:'midi',muted:false,solo:false,notes:[
      {step:0,row:7,len:2,vel:.78},{step:4,row:5,len:2,vel:.78},{step:8,row:3,len:2,vel:.78},{step:12,row:5,len:2,vel:.78},
      {step:16,row:7,len:2,vel:.78},{step:20,row:5,len:2,vel:.78},{step:24,row:3,len:2,vel:.78},{step:28,row:5,len:2,vel:.78}
    ]},
    {id:'drums',name:'Drums',type:'drum',muted:false,solo:false,notes:[]},
    {id:'bass',name:'Bass',type:'midi',muted:false,solo:false,notes:[]}
  ],
  recDismissed:false
};
let audioCtx=null, master=null, scheduler=null, nextNoteTime=0, currentStep=0, animationId=null;
const app=document.getElementById('app');

function save(){localStorage.setItem(STORAGE_KEY,JSON.stringify({...state,playing:false,positionStep:0}))}
function load(){try{const s=JSON.parse(localStorage.getItem(STORAGE_KEY));if(s)Object.assign(state,s)}catch{}}
function title(s){return s.charAt(0)+s.slice(1).toLowerCase()}
function escapeHtml(s){return String(s).replace(/[&<>'"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;',"'":'&#039;','"':'&quot;'}[c]))}
function goalDesc(g){return ({'Create my first song':'Start small and build an original idea.','Finish an idea':'Turn a loop into a complete song.','Learn production':'Create while Sensei teaches the concepts.','Improve a mix':'Focus on balance, clarity and musical intent.'})[g]}

function welcome(){
 app.innerHTML=`<section class="welcome"><div class="welcome-card"><div class="brand-row"><div class="logo">S</div><div><div class="eyebrow">Sensei Studio 0.3</div><strong>Functional prototype</strong></div></div><h1>Make something. Hear it immediately.</h1><p class="lead">This build has real browser audio, a working MIDI loop, BPM-controlled playback, mute/solo, and a tiny synth. Sensei now reacts to what you actually create.</p><div class="goal-grid">${['Create my first song','Finish an idea','Learn production','Improve a mix'].map((g,i)=>`<button class="goal ${i===0?'selected':''}" data-goal="${g}"><strong>${g}</strong><span>${goalDesc(g)}</span></button>`).join('')}</div><div class="row"><input id="projectName" class="field" value="${state.projectName}"/><button id="start" class="primary">Start creating →</button></div></div></section>`;
 document.querySelectorAll('.goal').forEach(b=>b.onclick=()=>{state.goal=b.dataset.goal;document.querySelectorAll('.goal').forEach(x=>x.classList.toggle('selected',x===b))});
 document.getElementById('start').onclick=async()=>{state.projectName=document.getElementById('projectName').value.trim()||'Untitled';state.started=true;await ensureAudio();save();renderStudio();toast('Audio is ready');};
}

async function ensureAudio(){
 if(!audioCtx){audioCtx=new (window.AudioContext||window.webkitAudioContext)();master=audioCtx.createGain();master.gain.value=.25;master.connect(audioCtx.destination)}
 if(audioCtx.state==='suspended') await audioCtx.resume();
}
function midiToFreq(m){return 440*Math.pow(2,(m-69)/12)}
function noteOn(midi,velocity=.8,duration=.25,when=null){
 if(!audioCtx||!master)return; when=when??audioCtx.currentTime;
 const osc=audioCtx.createOscillator(), filt=audioCtx.createBiquadFilter(), gain=audioCtx.createGain();
 osc.type=state.synth.wave;osc.frequency.value=midiToFreq(midi);filt.type='lowpass';filt.frequency.value=state.synth.filter;filt.Q.value=.7;
 const attack=.008, rel=Math.max(.05,state.synth.release); gain.gain.setValueAtTime(.0001,when);gain.gain.exponentialRampToValueAtTime(Math.max(.02,velocity),when+attack);gain.gain.setValueAtTime(Math.max(.02,velocity),when+Math.max(.03,duration));gain.gain.exponentialRampToValueAtTime(.0001,when+duration+rel);
 osc.connect(filt);filt.connect(gain);gain.connect(master);osc.start(when);osc.stop(when+duration+rel+.03);
}
function kick(when){if(!audioCtx)return;const o=audioCtx.createOscillator(),g=audioCtx.createGain();o.frequency.setValueAtTime(130,when);o.frequency.exponentialRampToValueAtTime(45,when+.12);g.gain.setValueAtTime(.8,when);g.gain.exponentialRampToValueAtTime(.001,when+.2);o.connect(g);g.connect(master);o.start(when);o.stop(when+.21)}
function hat(when){if(!audioCtx)return;const len=audioCtx.sampleRate*.04,b=audioCtx.createBuffer(1,len,audioCtx.sampleRate),d=b.getChannelData(0);for(let i=0;i<len;i++)d[i]=Math.random()*2-1;const s=audioCtx.createBufferSource(),f=audioCtx.createBiquadFilter(),g=audioCtx.createGain();s.buffer=b;f.type='highpass';f.frequency.value=6500;g.gain.setValueAtTime(.1,when);g.gain.exponentialRampToValueAtTime(.001,when+.04);s.connect(f);f.connect(g);g.connect(master);s.start(when)}

function renderStudio(){
 const track=state.tracks[state.selectedTrack];
 app.innerHTML=`<div class="studio"><header class="topbar"><div class="top-left"><div class="mini-logo">S</div><div><div class="project-name">${escapeHtml(state.projectName)}</div><div class="small">Sensei Studio · Functional 0.3</div></div></div><div class="top-center"><div class="mode-pill">${['EXPLORE','CREATE','ARRANGE','MIX','FINISH'].map(m=>`<button data-mode="${m}" class="${state.mode===m?'active':''}">${title(m)}</button>`).join('')}</div></div><div class="top-right"><span class="small">${state.goal}</span><button id="save" class="icon-btn">⌘</button></div></header>
 <main class="workspace"><aside class="sidebar"><div class="panel-title">Tracks</div><button id="addTrack" class="add-track">＋ Add MIDI track</button><div id="tracks">${trackList()}</div><div class="panel-title" style="margin-top:20px">Sensei Synth</div>${instrumentControls(track)}</aside>
 <section class="center"><div class="timeline"><div id="timelineHead" class="playhead" style="left:${10+state.positionStep*STEP_W}px"></div><div class="ruler">${Array.from({length:16},(_,i)=>`<div>${i+1}</div>`).join('')}</div><div class="lanes">${laneList()}</div></div><div class="editor"><div class="editor-tabs"><button class="tab active">Piano Roll</button><span class="small" style="margin-left:auto">Click = add/play · right-click a note = delete</span></div><div class="piano-wrap"><div class="keys">${keys()}</div><div id="roll" class="roll"><div id="pianoHead" class="piano-playhead" style="left:${state.positionStep*STEP_W}px"></div>${track.type==='midi'?noteList(track):'<div class="roll-hint">Select a MIDI track</div>'}</div></div></div></section>
 <aside class="sensei-panel">${senseiPanel()}</aside></main>
 <footer class="transport"><div class="tempo">BPM <input id="bpm" type="number" min="40" max="240" value="${state.bpm}"/></div><div class="transport-center"><button id="rew" class="icon-btn">↤</button><button id="play" class="play">${state.playing?'Ⅱ':'▶'}</button><button id="stop" class="icon-btn">■</button></div><div class="status">${state.playing?'Playing real audio':'Ready'} · 4/4 · 8-bar loop</div></footer></div><div id="toast" class="toast"></div>`;
 bindStudio();
}
function trackList(){return state.tracks.map((t,i)=>`<div class="track-item ${i===state.selectedTrack?'selected':''}" data-track="${i}"><span class="dot"></span><div><b>${escapeHtml(t.name)}</b><div class="small">${t.type==='drum'?'Drums':'MIDI'} · ${t.notes.length} notes</div></div><div class="track-actions"><button class="${t.muted?'on':''}" data-action="mute" data-i="${i}">M</button><button class="${t.solo?'on':''}" data-action="solo" data-i="${i}">S</button></div></div>`).join('')}
function laneList(){return state.tracks.map((t,i)=>`<div class="lane"><div class="clip" style="margin-left:${i*80}px;width:${Math.max(220,Math.min(520,220+t.notes.length*9))}px"><div class="clip-name">${escapeHtml(t.name)} idea</div><div class="mini-notes">${Array.from({length:12},(_,j)=>`<i style="height:${7+((j*11+t.notes.length*3)%20)}px"></i>`).join('')}</div></div></div>`).join('')}
function keys(){return NOTE_NAMES.map((k,row)=>`<div class="key ${k.includes('#')?'black':''}" data-key-row="${row}">${k}</div>`).join('')}
function noteList(track){return track.notes.map((n,i)=>`<div class="note" data-note="${i}" style="left:${n.step*STEP_W+2}px;top:${n.row*ROW_H+2}px;width:${Math.max(18,n.len*STEP_W-4)}px;opacity:${.45+n.vel*.55}"></div>`).join('')}
function instrumentControls(track){if(track.type!=='midi')return `<div class="instrument-card"><b>Drum track</b><p class="small">The demo drum engine plays a kick and hat pattern during playback.</p></div>`;return `<div class="instrument-card"><b>Tiny built-in synth</b><label>Waveform</label><select id="wave"><option ${state.synth.wave==='sine'?'selected':''}>sine</option><option ${state.synth.wave==='triangle'?'selected':''}>triangle</option><option ${state.synth.wave==='sawtooth'?'selected':''}>sawtooth</option><option ${state.synth.wave==='square'?'selected':''}>square</option></select><label>Brightness <span id="filterVal">${state.synth.filter} Hz</span></label><input id="filter" type="range" min="250" max="9000" step="50" value="${state.synth.filter}"><label>Release <span id="releaseVal">${state.synth.release.toFixed(2)} s</span></label><input id="release" type="range" min="0.05" max="1.2" step="0.05" value="${state.synth.release}"><p class="small">These three controls are intentionally simple. Later Sensei will explain what they map to in professional synths.</p></div>`}
function senseiPanel(){
 const t=state.tracks[state.selectedTrack], analysis=analyzeProject(); let card='';
 if(state.mode==='CREATE') card=`<div class="sensei-card"><h3>I’m staying mostly quiet.</h3><p>You’re creating. I’ll only mention something if it helps without breaking flow.</p>${analysis.createHint?`<div class="sensei-card highlight"><b>${analysis.createHint.title}</b><p>${analysis.createHint.text}</p></div>`:''}<button class="choice" data-mode-jump="MIX">Ask for a mix check</button></div>`;
 else if(state.mode==='ARRANGE') card=`<div class="sensei-card highlight"><div class="eyebrow">Real project observation</div><h3>${analysis.arrange.title}</h3><p>${analysis.arrange.text}</p><div class="choice-row"><button class="choice good" data-choice="keep">I like it like that</button><button class="choice" data-choice="work">Let’s do something</button></div></div>`;
 else if(state.mode==='MIX') card=`<div class="sensei-card highlight"><div class="eyebrow">Current project check</div><h3>${analysis.mix.title}</h3><p>${analysis.mix.text}</p><div class="choice-row"><button class="choice good" data-choice="keep">I like it like that</button><button class="choice" data-choice="work">Let’s work on it</button><button class="choice" data-choice="why">Why?</button></div></div>`;
 else if(state.mode==='FINISH') card=`<div class="sensei-card highlight"><h3>Song checkout</h3><p>${analysis.finish}</p><button class="choice">Keep creating</button></div>`;
 else card=`<div class="sensei-card"><h3>Explore.</h3><p>Click keys, add notes, change the waveform and filter, then listen to what changes.</p></div>`;
 return `<div class="sensei-head"><div class="sensei-badge"><div class="sensei-orb"></div><div><b>Sensei</b><div class="small">Producer mentor</div></div></div><span class="small">${title(state.mode)}</span></div>${card}<div class="panel-title">Live project facts</div><div class="quick-stats"><div class="stat"><b>${state.tracks.reduce((s,x)=>s+x.notes.length,0)}</b><span>MIDI notes</span></div><div class="stat"><b>${state.tracks.length}</b><span>tracks</span></div><div class="stat"><b>${state.bpm}</b><span>BPM</span></div><div class="stat"><b>${new Set(t.notes.map(n=>n.row)).size}</b><span>pitches used</span></div></div>`
}
function analyzeProject(){
 const midiTracks=state.tracks.filter(t=>t.type==='midi'), allNotes=midiTracks.flatMap(t=>t.notes), activePitches=new Set(allNotes.map(n=>n.row)), total=allNotes.length;
 const repeated=allNotes.length>=6 && activePitches.size<=2;
 const createHint= total===0?{title:'Start with one note.',text:'Click anywhere in the piano roll. You’ll hear it immediately.'}:total<4?{title:'You have a seed.',text:'Try adding a few more notes before judging the idea.'}:null;
 const arrange=repeated?{title:'Your idea repeats a very small pitch set.',text:'That can be intentional. If you want more movement, try changing only the ending of the phrase.'}:{title:'You already have some pitch movement.',text:`I can see ${activePitches.size} different pitches across ${total} MIDI notes. Try repeating the idea, then changing the last one or two notes.`};
 const emptyBass=state.tracks.find(t=>t.id==='bass')?.notes.length===0;
 const mix=emptyBass?{title:'Your bass track is still empty.',text:'Before worrying about low-end EQ, give the bass a musical job first. Arrangement before processing.'}:{title:'Your basic MIDI structure is ready for listening.',text:'At this stage I’d compare levels and note lengths before reaching for EQ or compression.'};
 const finish=`You currently have ${state.tracks.length} tracks and ${total} MIDI notes. ${emptyBass?'The bass track is empty, so I would not call the arrangement finished yet.':'You have material on the main MIDI roles.'}`;
 return {createHint,arrange,mix,finish};
}

function bindStudio(){
 document.querySelectorAll('[data-mode]').forEach(b=>b.onclick=()=>{state.mode=b.dataset.mode;save();renderStudio()});
 document.querySelectorAll('[data-mode-jump]').forEach(b=>b.onclick=()=>{state.mode=b.dataset.modeJump;save();renderStudio()});
 document.querySelectorAll('.track-item').forEach(el=>el.onclick=e=>{if(e.target.closest('button'))return;state.selectedTrack=+el.dataset.track;renderStudio()});
 document.querySelectorAll('[data-action]').forEach(b=>b.onclick=e=>{e.stopPropagation();const t=state.tracks[+b.dataset.i],key=b.dataset.action==='mute'?'muted':'solo';t[key]=!t[key];save();renderStudio()});
 document.getElementById('addTrack').onclick=()=>{state.tracks.push({id:'midi'+Date.now(),name:`MIDI ${state.tracks.length+1}`,type:'midi',muted:false,solo:false,notes:[]});state.selectedTrack=state.tracks.length-1;save();renderStudio()};
 document.getElementById('save').onclick=()=>{save();toast('Project saved in this browser')};
 document.getElementById('bpm').onchange=e=>{state.bpm=Math.max(40,Math.min(240,+e.target.value||94));save();renderStudio()};
 document.getElementById('play').onclick=async()=>{await ensureAudio();state.playing?stopPlayback(false):startPlayback()};
 document.getElementById('stop').onclick=()=>stopPlayback(true);document.getElementById('rew').onclick=()=>{state.positionStep=0;currentStep=0;updatePlayheads(0)};
 const wave=document.getElementById('wave');if(wave)wave.onchange=e=>{state.synth.wave=e.target.value;save();toast(`Waveform: ${e.target.value}`)};
 const filter=document.getElementById('filter');if(filter)filter.oninput=e=>{state.synth.filter=+e.target.value;document.getElementById('filterVal').textContent=`${state.synth.filter} Hz`;save()};
 const rel=document.getElementById('release');if(rel)rel.oninput=e=>{state.synth.release=+e.target.value;document.getElementById('releaseVal').textContent=`${state.synth.release.toFixed(2)} s`;save()};
 document.querySelectorAll('[data-key-row]').forEach(k=>k.onclick=async()=>{await ensureAudio();noteOn(MIDI_BY_ROW[+k.dataset.keyRow],.75,.18)});
 const roll=document.getElementById('roll');roll.onclick=async e=>{if(e.target.classList.contains('note'))return;const t=state.tracks[state.selectedTrack];if(t.type!=='midi')return;await ensureAudio();const rect=roll.getBoundingClientRect(),step=Math.max(0,Math.min(STEPS-1,Math.floor((e.clientX-rect.left+roll.scrollLeft)/STEP_W))),row=Math.max(0,Math.min(NOTE_NAMES.length-1,Math.floor((e.clientY-rect.top+roll.scrollTop)/ROW_H)));t.notes.push({step,row,len:2,vel:.78});noteOn(MIDI_BY_ROW[row],.78,.2);save();renderStudio()};
 document.querySelectorAll('.note').forEach(n=>{n.oncontextmenu=e=>{e.preventDefault();const t=state.tracks[state.selectedTrack];t.notes.splice(+n.dataset.note,1);save();renderStudio()}});
 document.querySelectorAll('[data-choice]').forEach(b=>b.onclick=()=>{const c=b.dataset.choice;if(c==='why')alert('Sensei separates observation from advice. It first notices something measurable in your actual project, then explains why it might matter. Your choice is still final.');else if(c==='work')toast('Good. We’ll turn this into a guided experiment next.');else toast('Got it — keeping your creative choice.')});
}
function audibleTrack(t){const anySolo=state.tracks.some(x=>x.solo);return !t.muted && (!anySolo||t.solo)}
function secondsPerStep(){return (60/state.bpm)/2}
function scheduleStep(step,when){
 state.tracks.forEach(t=>{if(!audibleTrack(t))return;if(t.type==='drum'){if(step%4===0)kick(when);if(step%2===0)hat(when);return;} t.notes.filter(n=>n.step===step).forEach(n=>noteOn(MIDI_BY_ROW[n.row],n.vel,n.len*secondsPerStep()*.78,when));});
}
function schedulerTick(){while(nextNoteTime<audioCtx.currentTime+.12){scheduleStep(currentStep,nextNoteTime);currentStep=(currentStep+1)%STEPS;nextNoteTime+=secondsPerStep()}scheduler=setTimeout(schedulerTick,25)}
function startPlayback(){state.playing=true;currentStep=state.positionStep||0;nextNoteTime=audioCtx.currentTime+.05;schedulerTick();animationStartTime=performance.now()-(currentStep*secondsPerStep()*1000);animatePlayhead();renderTransportOnly()}
let animationStartTime=0;
function animatePlayhead(){if(!state.playing)return;const elapsed=(performance.now()-animationStartTime)/1000,loopDur=STEPS*secondsPerStep(),pos=(elapsed%loopDur)/secondsPerStep();state.positionStep=Math.floor(pos)%STEPS;updatePlayheads(pos);animationId=requestAnimationFrame(animatePlayhead)}
function updatePlayheads(pos){const px=pos*STEP_W;const a=document.getElementById('timelineHead'),b=document.getElementById('pianoHead');if(a)a.style.left=`${10+px}px`;if(b)b.style.left=`${px}px`}
function stopPlayback(reset){state.playing=false;if(scheduler)clearTimeout(scheduler);scheduler=null;if(animationId)cancelAnimationFrame(animationId);animationId=null;if(reset){state.positionStep=0;currentStep=0;updatePlayheads(0)}renderTransportOnly()}
function renderTransportOnly(){const p=document.getElementById('play'),s=document.querySelector('.status');if(p)p.textContent=state.playing?'Ⅱ':'▶';if(s)s.textContent=`${state.playing?'Playing real audio':'Ready'} · 4/4 · 8-bar loop`}
function toast(msg){let t=document.getElementById('toast');if(!t)return;t.textContent=msg;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),1700)}

load(); state.started?renderStudio():welcome();
