#!/usr/bin/env python3
"""Generate 3DS icon/banner PNGs and a short silence WAV for CIA packaging."""
import os, wave, struct
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build')
os.makedirs(OUT, exist_ok=True)

BG = (24, 22, 20)
AMBER = (255, 168, 40)
GREEN = (80, 220, 120)
WHITE = (235, 232, 224)

def text_size(draw, text, font):
    bbox = draw.textbbox((0, 0), text, font=font)
    return bbox[2] - bbox[0], bbox[3] - bbox[1]

# --- icon 48x48 ---
icon = Image.new('RGB', (48, 48), BG)
d = ImageDraw.Draw(icon)
d.rectangle([1, 1, 46, 46], outline=AMBER, width=2)
d.line([8, 8, 8, 40], fill=GREEN, width=2)
for i in range(4):
    y = 14 + i * 8
    d.line([14, y, 40, y], fill=WHITE, width=2)
    if i < 3:
        d.line([14, y + 4, 40, y + 4], fill=(120, 116, 110), width=1)
icon.save(os.path.join(OUT, 'icon.png'))

# --- banner 256x128 ---
banner = Image.new('RGB', (256, 128), BG)
d = ImageDraw.Draw(banner)
d.rectangle([1, 1, 254, 126], outline=AMBER, width=3)
try:
    font_big = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 36)
    font_sm = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16)
except Exception:
    font_big = ImageFont.load_default()
    font_sm = ImageFont.load_default()

t1 = "AMENITY3DS"
w1, h1 = text_size(d, t1, font_big)
d.text(((256 - w1) // 2, 18), t1, font=font_big, fill=AMBER)
t2 = "LIVE BREAK SLICER"
w2, h2 = text_size(d, t2, font_sm)
d.text(((256 - w2) // 2, 78), t2, font=font_sm, fill=WHITE)
t3 = "CIA  |  CLK OUT 24PPQN"
w3, h3 = text_size(d, t3, font_sm)
d.text(((256 - w3) // 2, 100), t3, font=font_sm, fill=(120, 116, 110))
banner.save(os.path.join(OUT, 'banner.png'))

# --- silence wav (1 second, 22050 Hz mono 16-bit) ---
with wave.open(os.path.join(OUT, 'silence.wav'), 'wb') as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(22050)
    w.writeframes(b'\x00\x00' * 22050)

print("assets written to", OUT)
