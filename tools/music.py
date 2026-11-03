#!/usr/bin/env python3
"""Zankō 2006 ambient music, synthesised from scratch (no samples).
Writes 3 seamless loops to Data/Audio: city (menu + streets), hills (wilds), under (subway)."""
import numpy as np, sys, os, subprocess
from scipy.signal import fftconvolve, butter, sosfilt

SR = 22050
rng = np.random.default_rng(2003)
out_dir = sys.argv[1] if len(sys.argv) > 1 else "Data/Audio"
os.makedirs(out_dir, exist_ok=True)

def hz(m): return 440.0 * 2 ** ((m - 69) / 12)
def lp(x, f, o=2): return sosfilt(butter(o, f / (SR / 2), 'low', output='sos'), x)
def hp(x, f, o=2): return sosfilt(butter(o, f / (SR / 2), 'high', output='sos'), x)

def reverb(x, secs=4.5, mix=0.55, seed=0):
    r = np.random.default_rng(seed); n = int(SR * secs); t = np.arange(n) / SR
    ir = r.standard_normal(n) * np.exp(-t * 6.9 / secs); ir = lp(ir, 5000); ir /= np.sqrt((ir ** 2).sum())
    wet = fftconvolve(x, ir)[:len(x)]
    return x * (1 - mix) + wet * mix * 1.4

def wobble(n, depth, rate, seed):   # tape wow: slowly varying phase offset in seconds
    r = np.random.default_rng(seed); t = np.arange(n) / SR
    return depth * (np.sin(2 * np.pi * rate * t + r.uniform(0, 6)) + 0.5 * np.sin(2 * np.pi * rate * 2.7 * t + r.uniform(0, 6)))

def pad(notes, dur, detune=0.12, bright=1400, seed=0, wow=0.0):
    n = int(SR * dur); t = np.arange(n) / SR; w = wobble(n, wow, 0.23, seed); y = np.zeros(n)
    for m in notes:
        for d in (-detune, 0, detune):
            f = hz(m + d); ph = 2 * np.pi * f * (t + w)
            y += (2 * ((ph / (2 * np.pi)) % 1) - 1) * 0.6 + np.sin(ph) * 0.8   # saw + sine body
    env = np.minimum(1, np.minimum(t / (dur * 0.35), (dur - t) / (dur * 0.45)))
    return lp(y * np.clip(env, 0, 1), bright, 3) / (len(notes) * 3)

def bell(m, dur=3.5, amp=0.25):
    n = int(SR * dur); t = np.arange(n) / SR; f = hz(m)
    y = np.sin(2 * np.pi * f * t + 1.8 * np.exp(-t * 3) * np.sin(2 * np.pi * f * 3.5 * t)) * np.exp(-t * 1.6)
    return y * amp

def place(buf, x, at):
    i = int(at * SR) % len(buf); k = min(len(x), len(buf) - i); buf[i:i + k] += x[:k]
    if k < len(x): buf[:len(x) - k] += x[k:]   # wrap so the loop stays seamless

def loopify(x, tail):  # fold the reverb tail back onto the start
    body = x[:-tail].copy(); body[:tail] += x[-tail:]; return body

def finish(name, L, R):
    st = np.stack([L, R], 1); st /= np.max(np.abs(st)) / 0.8
    raw = os.path.join(out_dir, name + ".f32")
    st.astype(np.float32).tofile(raw)
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-f", "f32le", "-ar", str(SR), "-ac", "2", "-i", raw,
                    "-c:a", "libvorbis", "-q:a", "2", os.path.join(out_dir, name + ".ogg")], check=True)
    os.remove(raw); print(name, len(L) / SR, "s")

# ---------------- city: melancholic aero pads, sparse e-piano bells, tape wow
def city():
    chords = [[57, 64, 67, 71, 74], [53, 60, 64, 67, 72], [48, 55, 64, 67, 71], [55, 62, 65, 69, 74],
              [57, 64, 67, 72, 76], [50, 57, 62, 65, 69], [52, 59, 62, 67, 71], [55, 59, 62, 64, 69]]
    cd = 12.0; total = cd * len(chords); tail = int(SR * 5); n = int(SR * total) + tail
    out = []
    for ch in range(2):
        y = np.zeros(n)
        for i, c in enumerate(chords):
            p = pad(c, cd * 1.45, detune=0.09 + 0.03 * ch, bright=1100, seed=i * 7 + ch, wow=0.0016)
            place(y, p, i * cd)
            place(y, pad([c[0] - 12], cd * 1.3, 0.05, 260, seed=i + ch) * 1.6, i * cd)
        r = np.random.default_rng(11)   # same melody both channels
        for i, c in enumerate(chords):
            for k in range(r.integers(1, 4)):
                at = i * cd + r.uniform(1, cd - 2); m = c[r.integers(1, len(c))] + 12
                place(y, bell(m, 4, 0.16) * (0.8 if ch else 1.0), at + ch * 0.012)
        y += lp(np.random.default_rng(ch).standard_normal(n), 3000) * 0.01   # tape hiss
        y = reverb(y, 5, 0.6, seed=ch); out.append(loopify(y, tail))
    finish("music_city", *out)

# ---------------- hills: bright major-7 Bliss chords that slowly go wrong (pitch sag, reversed swells)
def hills():
    chords = [[60, 64, 67, 71, 76], [65, 69, 72, 76, 79], [62, 66, 69, 73, 78], [67, 71, 74, 78, 83]]
    cd = 16.0; total = cd * len(chords); tail = int(SR * 6); n = int(SR * total) + tail
    out = []
    for ch in range(2):
        y = np.zeros(n)
        for i, c in enumerate(chords):
            p = pad(c, cd * 1.4, detune=0.18 + 0.05 * ch, bright=2600, seed=30 + i * 5 + ch, wow=0.006)
            place(y, p, i * cd)
            sw = pad([c[-1] + 12, c[-2] + 11.6], 5, 0.3, 3000, seed=90 + i)[::-1] * 0.9   # reversed, slightly flat swell
            place(y, sw, i * cd + cd - 5.5)
        t = np.arange(n) / SR
        y += np.sin(2 * np.pi * hz(36) * t) * 0.05 * (0.5 + 0.5 * np.sin(2 * np.pi * t / total))   # sub hum under the brightness
        y = reverb(y, 7, 0.7, seed=5 + ch); out.append(loopify(y, tail))
    finish("music_hills", *out)

# ---------------- under: subway drone, distant metal, far train rumble
def under():
    total = 80.0; tail = int(SR * 6); n = int(SR * total) + tail; t = np.arange(n) / SR
    out = []
    for ch in range(2):
        y = (np.sin(2 * np.pi * 43.65 * t) + 0.6 * np.sin(2 * np.pi * (44.1 + 0.2 * ch) * t) + 0.3 * np.sin(2 * np.pi * 65.4 * t)) * 0.18
        y *= 0.7 + 0.3 * np.sin(2 * np.pi * t / 20 + ch)
        y += lp(np.random.default_rng(40 + ch).standard_normal(n), 140, 3) * 0.6 * (0.4 + 0.6 * np.clip(np.sin(2 * np.pi * (t - 30) / total), 0, 1) ** 2)
        r = np.random.default_rng(77)
        for k in range(14):   # pipes ticking, rail creak: inharmonic metal pings
            at = r.uniform(0, total); f0 = r.uniform(300, 1400); d = 2.5; tt = np.arange(int(SR * d)) / SR
            ping = sum(np.sin(2 * np.pi * f0 * q * tt) * np.exp(-tt * (2 + q)) for q in (1, 2.76, 5.4)) * 0.09
            place(y, ping * (0.6 if (k + ch) % 2 else 1.0), at)
        y = hp(y, 30); y = reverb(y, 6, 0.65, seed=12 + ch); out.append(loopify(y, tail))
    finish("music_under", *out)

city(); hills(); under()
