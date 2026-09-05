#!/usr/bin/env python3
"""Reproducible, isolated projectM extension and authored adaptations."""
import hashlib, json, pathlib, re, shutil, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE = ROOT/'cache/projectm-upstream'
OUT = ROOT/'cache/milkdrop-audio-pilot'
SRC = OUT/'projectm'
if not SRC.exists():
    shutil.copytree(BASE, SRC, ignore=shutil.ignore_patterns('.git'))

def write_changed(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)

def patch(name, old, new):
    p = SRC/'src/libprojectM'/name
    # Always start from the preserved upstream source, allowing repeat builds.
    s = (BASE/'src/libprojectM'/name).read_text()
    assert old in s, (name, old)
    write_changed(p, s.replace(old, new, 1))

patch('ProjectM.hpp', '    void RenderFrame();', '''    void RenderFrame();
    // Experimental per-instance input. ABI is private to the isolated pilot.
    std::array<float, 8> omadropAudio{};''')
patch('Audio/FrameAudioData.hpp', '    float bass{0.f}', '    std::array<float, 8> omadropAudio{};\n    float bass{0.f}')
patch('ProjectM.cpp', 'auto audioData = m_audioStorage.GetFrameAudioData();', 'auto audioData = m_audioStorage.GetFrameAudioData();\n    audioData.omadropAudio = omadropAudio;')
patch('ProjectMCWrapper.cpp', '#include <cstring>', '''#include <cstring>
#include <cmath>
#include <algorithm>
extern "C" PROJECTM_EXPORT void projectm_set_omadrop_audio(projectm_handle instance, const float* values, unsigned count)
{
    if (!instance || !values || count != 8) return;
    auto* player = reinterpret_cast<libprojectM::projectMWrapper*>(instance);
    for (unsigned i = 0; i < 8; ++i)
        player->omadropAudio[i] = std::isfinite(values[i]) ? std::clamp(values[i], 0.0f, 1.0f) : 0.0f;
}
''')
patch('MilkdropPreset/PerFrameContext.hpp', '    PRJM_EVAL_F* zoom{};', '    PRJM_EVAL_F* omVars[8]{};\n    PRJM_EVAL_F* zoom{};')
p = SRC/'src/libprojectM/MilkdropPreset/PerFrameContext.cpp'
s = (BASE/'src/libprojectM/MilkdropPreset/PerFrameContext.cpp').read_text()
s = s.replace('    REG_VAR(zoom);', '''    const char* names[] = {"om_mix", "om_low", "om_mid", "om_high", "om_attack", "om_tone", "om_air", "om_energy"};
    for (int i = 0; i < 8; ++i)
        omVars[i] = projectm_eval_context_register_variable(perFrameCodeContext, names[i]);
    REG_VAR(zoom);''', 1)
s = s.replace('    *zoom = static_cast', '    for (int i = 0; i < 8; ++i) *omVars[i] = state.audioData.omadropAudio[i];\n    *zoom = static_cast', 1)
write_changed(p, s)

manifest = json.loads((ROOT/'experiments/milkdrop-audio-pilot/manifest.json').read_text())
presets = OUT/'presets'; presets.mkdir(parents=True, exist_ok=True)
for entry in manifest['presets']:
    original = ROOT/entry['original_path']
    assert hashlib.sha256(original.read_bytes()).hexdigest() == entry['sha256']
    s = original.read_text()
    def replace(old, new):
        global s
        assert old in s, (entry['label'], old)
        s = s.replace(old, new, 1)
    def append_frame(code):
        global s
        import re
        index = max(map(int, re.findall(r'^per_frame_(\d+)=', s, re.M))) + 1
        s += f'\nper_frame_{index}={code}\n'
    n = entry['number']
    # New shader inputs must not overwrite an authored q variable, including
    # reads in custom waves/shapes. Aqua's q11 is already used by its waves.
    reserved = {6: [21,22,23], 7: [9,10,11,12], 8: [9,10,12], 10: [1], 11: [15,16], 12: [9], 13: [11,12], 14: [2,3,4], 15: [18], 16: [1,2,3,4], 17: [29,32,15], 18: [2], 20: [1]}
    for q in reserved.get(n, []):
        assert not re.search(r'\bq'+str(q)+r'\b', s), (entry['label'], q)
    if n == 1:
        # Direct stereo band envelopes, without a second slow size filter.
        append_frame('q13=(1-om_mix)*q3+om_mix*(.078+.09*om_low); q14=(1-om_mix)*q3+om_mix*(.078+.09*om_mid); q15=(1-om_mix)*q3+om_mix*(.078+.09*om_high); q16=om_mix*om_low; q17=om_mix*om_tone; q18=om_mix*om_air;')
        for cube, spin in enumerate([6,9,12]):
            replace(f'shape_{cube}_per_frame5=rad = q3*sqrt(2);', f'shape_{cube}_per_frame5=rad = q{13+cube}*sqrt(2);')
            replace(f'shape_{cube}_per_frame1=an = an + q{spin};', f'shape_{cube}_per_frame1=an = an + q{spin} + .003*q{16+cube}*60/max(fps,20);')
    elif n == 2:
        # Open the fractal's folds, not its camera zoom. Both navigation and shader
        # must use identical fold limits; density and original camera orientation stay intact.
        replace('per_frame_1=', 'per_frame_1=om_flow=(om_flow+om_tone*.8/max(fps,20))%6.283185; om_fx=om_fx+(om_tone-om_fx)*(1-exp(-10/max(fps,20))); om_fy=om_fy+(om_low-om_fy)*(1-exp(-10/max(fps,20))); q15=1+om_mix*(.12*om_fx+.055*om_fx*sin(om_flow)); q18=1+om_mix*.12*om_fy; ')
        for axis,q in [('x',15),('y',18)]:
            replace(f'uv{axis} = if (uv{axis} > 1, 2-uv{axis}, if(uv{axis} < -1, -2-uv{axis}, uv{axis}));', f'uv{axis} = if (uv{axis} > q{q}, 2*q{q}-uv{axis}, if(uv{axis} < -q{q}, -2*q{q}-uv{axis}, uv{axis}));')
            # Above replacement hits initialization first. Adapt the frame navigator too.
            replace(f'uv{axis} = if (uv{axis} > 1, 2-uv{axis}, if(uv{axis} < -1, -2-uv{axis}, uv{axis}));', f'uv{axis} = if (uv{axis} > q{q}, 2*q{q}-uv{axis}, if(uv{axis} < -q{q}, -2*q{q}-uv{axis}, uv{axis}));')
        replace('per_frame_init_1=', 'per_frame_init_1=q15=1; q18=1; om_px=1; om_py=1; ')
        replace('zz = 2.0*clamp(zz,-1,1)-zz;', 'zz = 2.0*clamp(zz,-float3(q15,q18,1),float3(q15,q18,1))-zz;')
        replace('speed = min', 'speed = (1+om_mix*.5*om_energy)*min')
        append_frame('q14=q14+if(above(frame,1),min(.1,3*(abs(q15-om_px)+abs(q18-om_py))),0); om_px=q15; om_py=q18; q30=om_mix*om_air;')
        replace('float grad = length(float2(dx.x,dy.x));', 'float grad = length(float2(dx.x,dy.x))*(1+.4*q30);')
    elif n == 3:
        # Sustained low/tone movement shapes the reflected surface; air reveals crests.
        append_frame('q5=om_mix*(.5*om_low+.25*om_tone); q6=om_mix*.25*om_air;')
        replace('uv3 += dz * (1-mask);', 'uv3 += dz * (1-mask)*(1+q5);')
        replace('ret += saturate (q22*mus2*fmask);', 'ret += saturate ((q22+q6)*mus2*fmask);')
    elif n == 4:
        # Musical activity changes continuous ribbon travel, never instantaneous phase.
        replace('mytime=mytime+(1/FPS)*(1+addtime);', 'mytime=mytime+(1/FPS)*(1+addtime+om_mix*(1.1*om_tone+.5*om_air));')
        append_frame('q2=q2*(1-om_mix*.2*om_low);')
    elif n == 5:
        # Keep reaction chemistry stable; reshape the relief and move its illumination.
        append_frame('om_flow=(om_flow+om_tone*.9/max(fps,20))%6.283185; q1=om_mix*(1.1*om_low+.4*om_tone); q2=om_mix*.35*sin(om_flow)*om_tone; q3=om_mix*.25*om_air; q4=om_mix*.2*cos(om_flow)*om_tone;')
        replace('N.z = -0.12;', 'N.z = -0.12/(1+q1);')
        replace('float3(q6,q7,-0.8)', 'float3(q6+q2,q7+q4,-0.8)')
        replace('pow(saturate(dot(R,L)),32)*0.5', 'pow(saturate(dot(R,L)),32*(1+q3))*0.5')
    elif n == 6:
        # Preserve the fluid reaction constants. Modulate existing local advection
        # and separate material refraction, rather than zooming the whole image.
        append_frame('q21=om_mix*.35*om_low; q22=om_mix*.4*om_tone; q23=om_mix*.3*om_air;')
        replace('m1 = q11*25;', 'm1 = q11*25*(1+q21);')
        replace('float2(dx.y,dy.y)*texsize.zw*32;', 'float2(dx.y,dy.y)*texsize.zw*32*(1+q22);')
        replace('GetBlur2(uv + float2(dx.y,dy.y)*0.1).y*2', 'GetBlur2(uv + float2(dx.y,dy.y)*0.1*(1+q23)).y*2')
    elif n == 7:
        # Ring spacing is geometric; travel is integrated so phase never jumps.
        append_frame('om_travel=(om_travel+om_mix*.055*om_tone/max(fps,20))%1; om_turn=(om_turn+om_mix*.018*om_air/max(fps,20))%1; q9=om_mix*.12*om_low; q10=om_mix*.25*om_air; q11=om_travel; q12=om_turn;')
        replace('float rad1 = .1/rad2 ;', 'float rad1 = .1*(1+q9)/rad2 ;')
        replace('uv2.y = uv2.y  +0.1*time;', 'uv2.y = uv2.y  +0.1*time+q11;')
        replace('uv2.x = uv2.x  +.02*time;', 'uv2.x = uv2.x  +.02*time+q12;')
        replace('mus = .1/(sqrt(uv6.y-.2));', 'mus = .1/(sqrt(uv6.y-(.2+.04*q10)));')
    elif n == 8:
        # Each existing depth layer gets a different response. Preserve its
        # original crystal generator and avoid occupying q11, used by waves.
        replace('movez = movez + .0035*(q1+1.1)*30/fps;', 'movez = movez + .0035*(q1+1.1)*(1+om_mix*.65*om_tone)*30/fps;')
        replace('rota = rota + .001*(2-q1)*30/fps;', 'rota = rota + .001*(2-q1)*(1+om_mix*.5*om_air)*30/fps;')
        append_frame('q9=om_mix*.1*om_low; q10=om_mix*.18*om_air; q12=om_mix*.3*om_tone;')
        replace('uv3 = 3*uv2*dist*1.25;', 'uv3 = 3*uv2*dist*1.25*(1+q9*cos(6.28*n/anz));')
        replace('mus = .25*abs(0.0225/(sqrt(uv6.x)+.0))*(.3);', 'mus = .25*abs(0.0225/(sqrt(uv6.x)+.0))*(.3)*(1+q12);')
        replace('(crisp*2+blur)*inten', '(crisp*(2+q10)+blur)*inten')
    elif n == 9:
        # Change particle birth velocities, never population-wide scale/camera.
        # Counts, lifetimes, pools and the original emission logic stay intact.
        replace('per_frame_1=', 'per_frame_1=pSpeed=.07*(1+om_mix*(.65*om_low+.3*om_tone)); tSpeed=hSpeed*3.5*(1+om_mix*.55*om_tone); ')
        replace('gmegabuf(self+13) = rand(100)/100+.5;//size', 'gmegabuf(self+13) = (rand(100)/100+.5)*(1+om_mix*.25*om_air);//size')
        replace('q13 = max(0,q13);', 'q13 = max(0,q13)*(1+om_mix*.3*om_air);')
    elif n == 10:
        # Strengthen the existing flow and texture injection; preserve its
        # warm palette, feedback decay and authored direction changes.
        append_frame('rot=rot*(1+om_mix*.35*om_tone); warp=warp*(1+om_mix*.45*om_low); wave_a=wave_a*(1+om_mix*.3*om_air); q1=om_mix*.35*om_tone;')
        replace('ret = lerp(ret, pic, use_it*0.07);', 'ret = lerp(ret, pic, use_it*0.07*(1+q1));')
    elif n == 11:
        replace('t = t + 0.1/fps;', 't = t + (0.1+om_mix*.065*om_tone)/fps;')
        append_frame('q15=om_mix*.35*om_low; q16=om_mix*.2*om_air;')
        replace('v = -15*q5;', 'v = -15*q5*(1+q15);')
        replace('(lm-0.35)*0.0168*(lm-0.4)', '(lm-0.35)*0.0168*(lm-0.4)*(1+q16)')
    elif n == 12:
        replace('movez = movez + .006*(q1+1.1)*30/fps;', 'movez = movez + .006*(q1+1.1)*(1+om_mix*.55*om_tone)*30/fps;')
        replace('rota = rota + .003*(2-q1)*30/fps;', 'rota = rota + .003*(2-q1)*(1+om_mix*.4*om_air)*30/fps;')
        append_frame('q9=om_mix*.09*om_low;')
        replace('uv3 = 3*uv2*dist + 0.5', 'uv3 = 3*uv2*dist*(1+q9*cos(6.28*n/anz)) + 0.5')
    elif n == 13:
        append_frame('q11=om_mix*(.3*om_low+.2*om_tone); q12=om_mix*.18*om_air;')
        replace('uv4+.2*dz', 'uv4+.2*(1+q11)*dz')
        replace('64*dz;', '64*(1+q12)*dz;')
    elif n == 14:
        # Leave the authored beat warp and reaction constants intact.
        append_frame('q2=om_mix*.3*om_low; q3=om_mix*.3*om_tone; q4=om_mix*.25*om_air;')
        replace('float2(dx.y,dy.y)*0.3', 'float2(dx.y,dy.y)*0.3*(1+q2)')
        replace('my_uv = uv - float2(dx.y,dy.y)*0.01', 'my_uv = uv - float2(dx.y,dy.y)*0.01*(1+q3)')
        replace('float2(dx.x,dy.x)*texsize.zw*18', 'float2(dx.x,dy.x)*texsize.zw*18*(1+q4)')
    elif n == 15:
        replace('movz = movz + 2/fps*speed_;', 'movz = movz + 2/fps*speed_*(1+om_mix*.5*om_tone);')
        replace('trel = trel + .2/fps*dir;', 'trel = trel + .2/fps*dir*(1+om_mix*.4*om_air);')
        append_frame('q18=om_mix*.08*om_low;')
        replace('q13*dist*float2x2(c,s,-s,c)', 'q13*dist*(1+q18*cos(6.28*n/anz))*float2x2(c,s,-s,c)')
    elif n == 16:
        append_frame('om_phase=om_phase+om_mix*.35*om_tone/max(fps,20); q1=om_mix*.15*om_mid; q2=om_mix*.15*om_low; q3=om_mix*.15*om_high; q4=om_phase;')
        for shape,q,phase in [(1,1,''),(2,2,' + 2.09'),(3,3,' + 4.19')]:
            replace(f'shape_{shape}_per_frame2=x = 0.5 + 0.225*sin(time{phase});', f'shape_{shape}_per_frame2=x = 0.5 + 0.225*(1+q{q})*sin(time+q4{phase});')
            replace(f'shape_{shape}_per_frame3=y = 0.5 + 0.3*cos(time{phase});', f'shape_{shape}_per_frame3=y = 0.5 + 0.3*(1+q{q})*cos(time+q4{phase});')
    elif n == 17:
        # As in the accepted cave, geometry and navigation must agree. This
        # faster flight uses smaller fold changes and retains its travel speed.
        replace('per_frame_init_1=', 'per_frame_init_1=q29=1; q32=1; om_px=1; om_py=1; ')
        replace('per_frame_1=', 'per_frame_1=om_fx=om_fx+(om_tone-om_fx)*(1-exp(-8/max(fps,20))); om_fy=om_fy+(om_low-om_fy)*(1-exp(-8/max(fps,20))); q29=1+om_mix*.065*om_fx; q32=1+om_mix*.06*om_fy; ')
        for axis,q in [('x',29),('y',32)]:
            old=f'uv{axis} = if (uv{axis} > 1, 2-uv{axis}, if(uv{axis} < -1, -2-uv{axis}, uv{axis}));'
            assert s.count(old)==2
            for _ in range(2):replace(old,f'uv{axis} = if (uv{axis} > q{q}, 2*q{q}-uv{axis}, if(uv{axis} < -q{q}, -2*q{q}-uv{axis}, uv{axis}));')
        replace('zz = 2.0*clamp(zz,-1,1)-zz;', 'zz = 2.0*clamp(zz,-float3(q29,q32,1),float3(q29,q32,1))-zz;')
        append_frame('q14=q14+min(.08,3*(abs(q29-om_px)+abs(q32-om_py))); om_px=q29; om_py=q32; q15=om_mix*.15*om_air;')
        replace('float struc = GetBlurX(uv,focus).r;', 'float struc = GetBlurX(uv,focus/(1+q15)).r;')
    elif n == 18:
        replace('mtime=mtime+vol*0.1;', 'mtime=mtime+vol*0.1+om_mix*.5*om_tone/max(fps,20);')
        append_frame('q2=om_mix*(.25*om_low+.2*om_air);')
        replace('(N.xy*2-1)*texsize.zw*13;', '(N.xy*2-1)*texsize.zw*13*(1+q2);')
    elif n == 19:
        replace('per_frame_1=', 'per_frame_1=pSpeed=.06*(1+om_mix*(.45*om_low+.25*om_tone)); tSpeed=hSpeed*3.5*(1+om_mix*.4*om_tone); ')
        replace('gmegabuf(self+13) = rand(100)/100+.5;//size', 'gmegabuf(self+13) = (rand(100)/100+.5)*(1+om_mix*.2*om_air);//size')
    elif n == 20:
        replace('tt=tt+bass*.01;', 'tt=tt+bass*.01+om_mix*.3*om_tone/max(fps,20);')
        append_frame('q1=om_mix*(.09*om_low+.04*om_air);')
        replace('rad=.9+q2*.1-q6*.1;', 'rad=(.9+q2*.1-q6*.1)*(1+q1);')
    elif n == 21:
        # Independent integrated clocks retain each stick's articulated shape.
        replace('q1=time;', 'om_t1=om_t1+om_mix*.45*om_low/max(fps,20); q1=time+om_t1;')
        replace('q2=time;', 'om_t2=om_t2+om_mix*.5*om_tone/max(fps,20); q2=time+om_t2;')
        replace('q3=time;', 'om_t3=om_t3+om_mix*.5*om_air/max(fps,20); q3=time+om_t3;')
        replace('wave_2_per_point3=tm=q1 + phs;', 'wave_2_per_point3=tm=q2 + phs;')
        replace('wave_3_per_point3=tm=q1 + phs;', 'wave_3_per_point3=tm=q3 + phs;')
    else:
        raise ValueError(f"No adaptation for {entry['label']}")
    write_changed(presets/entry['preset'], s)
print(OUT)

# Generate an isolated player translation unit; shared baseline sources are untouched.
s = (ROOT/'experiments/projectm-ascii/live.cpp').read_text()
s = '#include "audio_controls.h"\nextern "C" void projectm_set_omadrop_audio(void*, const float*, unsigned);\n' + s
s = s.replace('    MusicFrame musicFrame;', '''    MusicFrame musicFrame;
    PilotAudioControls pilotControls;
    bool pilotEnabled = !std::getenv("OMADROP_PILOT_ORIGINAL");
    std::size_t pilotLabelIndex = static_cast<std::size_t>(-1);
    bool pilotLabelEnabled = !pilotEnabled;
    std::uint64_t pilotLastSync = 0;
    std::array<float, 8> pilotSynchronizedAudio{};
    std::ofstream pilotLog;
    if (const char* path = std::getenv("OMADROP_PILOT_LOG")) pilotLog.open(path);
''', 1)
s = s.replace('            if (!calibrationMode && event.type == SDL_KEYDOWN\n                && event.key.keysym.sym == SDLK_n)', '''            if (event.type == SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym == SDLK_o)
            {
                if (pairedFollower) pairedControlRequest = "collection-response";
                else pilotEnabled = !pilotEnabled;
            }
            if (!calibrationMode && event.type == SDL_KEYDOWN
                && event.key.keysym.sym == SDLK_n)''', 1)
assert 'pilotEnabled = !pilotEnabled' in s
s = s.replace('            structureClockLocked = structure.clockLocked;', '            pilotControls.processStereo(pcm.data(), analysisHopFrames, musicFrame);\n            structureClockLocked = structure.clockLocked;', 1)
s = s.replace('musicFrameBuilder.reset();', 'musicFrameBuilder.reset(); pilotControls.resetAudio();')
s = s.replace('        auto renderEngine = [&](int index) {', '''        if (const char* toggleMs = std::getenv("OMADROP_PILOT_TOGGLE_MS")) {
            static const auto toggleStart = now;
            pilotEnabled = ((now - toggleStart) / std::max(1, std::atoi(toggleMs))) % 2 == 0;
        }
        auto pilotAudio = pilotControls.update(musicFrame, frameSeconds, pilotEnabled);
        if (pairedLeader) {
            pairedTransport.publishMusic(PairedMusicState{
                .serial = ++pairedMusicSerial, .frame = musicFrame,
                .collectionControls = pilotAudio, .collectionResponseEnabled = pilotEnabled});
        } else if (pairedFollower) {
            if (const auto sync = pairedMusicFollower.consume(pairedTransport.readMusic())) {
                pilotSynchronizedAudio = sync->collectionControls;
                pilotAudio = sync->collectionControls;
                pilotEnabled = sync->collectionResponseEnabled;
                pilotLastSync = now;
                if (!reportedPairedMusic) {
                    std::cerr << "paired collection: synchronized audio controls\\n";
                    reportedPairedMusic = true;
                }
            } else if (pilotLastSync && now - pilotLastSync < 250) {
                pilotAudio = pilotSynchronizedAudio;
            } else {
                pilotAudio.fill(0);
            }
        }
        if (pilotLog) {
            pilotLog << now << ',' << presetIndex;
            for (float v : pilotAudio) pilotLog << ',' << v;
            pilotLog << '\\n';
        }
        for (auto engine : engines) projectm_set_omadrop_audio(engine, pilotAudio.data(), pilotAudio.size());
        auto renderEngine = [&](int index) {''', 1)
s = s.replace('        if (!statusOverlay.render(outputW, outputH, now, compositorError)) {', '''        if (pilotLabelIndex != presetIndex || pilotLabelEnabled != pilotEnabled) {
            const bool adapted = std::filesystem::path(presets[presetIndex]).parent_path().filename() == "pilot";
            const std::string label = std::filesystem::path(presets[presetIndex]).stem().string()
                + (adapted && pilotEnabled ? "  ENGINE ON" : "  ORIGINAL RESPONSE");
            // Normal rotation stays unobstructed. Only explicitly toggling comparison shows a label.
            if (pilotLabelIndex != static_cast<std::size_t>(-1) && pilotLabelEnabled != pilotEnabled)
                statusOverlay.show(label, now, 2500);
            SDL_SetWindowTitle(window, label.c_str());
            pilotLabelIndex = presetIndex;
            pilotLabelEnabled = pilotEnabled;
        }
        if (!statusOverlay.render(outputW, outputH, now, compositorError)) {''', 1)
assert s.count('pilotControls.processStereo(pcm.data(), analysisHopFrames, musicFrame);') == 1
assert s.count('projectm_set_omadrop_audio(engine, pilotAudio.data(), pilotAudio.size())') == 1
assert s.count('pilotControls.resetAudio();') == s.count('musicFrameBuilder.reset();')
s = s.replace('                if (request == "ascii") {',
    '                if (request == "collection-response") { pilotEnabled = !pilotEnabled; }\n'
    '                else if (request == "ascii") {', 1)
assert 'request == "collection-response"' in s
write_changed(OUT/'live.cpp', s)
