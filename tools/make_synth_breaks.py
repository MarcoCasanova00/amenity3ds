#!/usr/bin/env python3
"""Generate original (CC0) synthesized drum breaks for the public builds.

These are entirely original percussion synthesized here — no third-party
audio — so the public release CIA / GBC ROM can be redistributed freely.
Patterns are two-bar loops at 160 BPM (~3.0 s at 44.1 kHz).
"""
import os, sys, wave, math

SR = 44100
BPM = 160
EIGHTH = 60.0 / BPM / 2.0      # seconds per 8th note
STEPS = 16
DUR = STEPS * EIGHTH            # 3.0 s
N = int(SR * DUR)

def env_decay(n, tau):
    return math.exp(-n / (SR * tau))

def kick(n):
    f = 120.0
    ph = 0.0
    out = []
    for i in range(n):
        ph += f / SR
        out.append(math.sin(2 * math.pi * ph) * env_decay(i, 0.09) * 0.9)
        f = f * 0.996 + 40.0 * 0.004
    return out

def noise_burst(n, tau, hp=True):
    out = []
    last = 0.0
    for i in range(n):
        r = random_01() * 2 - 1
        if hp:
            r = r - last
            last = r
        out.append(r * env_decay(i, tau))
    return out

def snare(n):
    out = []
    ph = 0.0
    for i in range(n):
        ph += 180.0 / SR
        out.append(math.sin(2 * math.pi * ph) * env_decay(i, 0.04) * 0.5)
    nb = noise_burst(n, 0.06)
    return [out[i] + nb[i] * 0.8 for i in range(n)]

_random_state = 12345
def random_01():
    global _random_state
    _random_state = (_random_state * 1103515245 + 12345) & 0x7fffffff
    return _random_state / 0x7fffffff

def build_pattern(seed_offset):
    buf = [0.0] * N
    # two bar patterns, two variants
    if seed_offset == 0:
        grid = [(0,'K'),(2,'C'),(4,'K'),(6,'C'),(7,'S'),(8,'K'),(10,'C'),
                (12,'K'),(13,'C'),(14,'O'),(15,'S')]
    else:
        grid = [(0,'K'),(1,'C'),(3,'S'),(4,'K'),(6,'C'),(7,'O'),(8,'K'),
                (10,'C'),(11,'S'),(12,'K'),(14,'C'),(15,'C')]
    for step, kind in grid:
        start = int(step * EIGHTH * SR)
        n = N - start
        if kind == 'K':
            m = min(n, int(0.35 * SR))
            seg = kick(m)
            for i in range(m): buf[start+i] += seg[i] * 1.0
        elif kind == 'S':
            m = min(n, int(0.25 * SR))
            seg = snare(m)
            for i in range(m): buf[start+i] += seg[i] * 0.9
        elif kind == 'O':
            m = min(n, int(0.25 * SR))
            seg = noise_burst(m, 0.09)
            for i in range(m): buf[start+i] += seg[i] * 0.55
        else:  # closed hat
            m = min(n, int(0.06 * SR))
            seg = noise_burst(m, 0.012)
            for i in range(m): buf[start+i] += seg[i] * 0.4
    # soft normalize
    peak = max(abs(v) for v in buf) or 1
    g = 0.9 / peak
    return [int(max(-32768, min(32767, v * g * 32767))) for v in buf]

def write_wav(path, samples):
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b''.join(v.to_bytes(2, 'little', signed=True) for v in samples))

def main():
    out = sys.argv[1] if len(sys.argv) > 1 else 'assets/synth_breaks'
    os.makedirs(out, exist_ok=True)
    # seed rng deterministically per variant
    for i, name in enumerate(['SYNTH1', 'SYNTH2', 'SYNTH3']):
        global _random_state
        _random_state = 12345 + i * 777
        samples = build_pattern(i)
        write_wav(os.path.join(out, name + '.wav'), samples)
        print('wrote %s' % os.path.join(out, name + '.wav'))

if __name__ == '__main__':
    main()
