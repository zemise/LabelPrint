#!/usr/bin/env python3
"""Barcode label preview — local PIL renderer with CJK support."""

import sys, os, argparse, io
from PIL import Image, ImageDraw, ImageFont
import barcode
from barcode.writer import ImageWriter

LABEL_W = 400   # dots @ 203 DPI → 5.0 cm
LABEL_H = 240   # dots @ 203 DPI → 3.0 cm
DPI     = 203
SCALE   = 3

# ---------------------------------------------------------------------------
def _find_cjk_font() -> str:
    candidates = [
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        "/Library/Fonts/Arial Unicode.ttf",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
        "C:/Windows/Fonts/simsun.ttc",
        "C:/Windows/Fonts/msyh.ttc",
    ]
    for p in candidates:
        if os.path.exists(p):
            return p
    return ""

CJK_FONT_PATH = _find_cjk_font()

def cjk_font(size: int) -> ImageFont.FreeTypeFont:
    if CJK_FONT_PATH:
        return ImageFont.truetype(CJK_FONT_PATH, size)
    return ImageFont.load_default()

# ---------------------------------------------------------------------------
def draw_text(draw, x, y, text, size_dots, scale):
    font = cjk_font(int(size_dots * scale * 0.85))
    draw.text((x * scale, y * scale), text, fill=0, font=font)

def draw_barcode_img(draw, x, y, data, bar_h, mod_w, scale):
    options = {
        "module_width":  mod_w * scale / DPI * 25.4,
        "module_height": bar_h * scale / DPI * 25.4 * 0.8,
        "font_size":     int(7 * scale),
        "text_distance":  1 * scale,
        "quiet_zone":     2 * scale,
        "write_text":    False,
    }
    buf = io.BytesIO()
    code = barcode.get("code128", data, writer=ImageWriter())
    code.write(buf, options)
    buf.seek(0)
    barcode_img = Image.open(buf).convert("L")

    target_w = int(LABEL_W * scale * 0.92)
    if barcode_img.width > target_w:
        ratio = target_w / barcode_img.width
        barcode_img = barcode_img.resize(
            (target_w, int(barcode_img.height * ratio)), Image.NEAREST)

    paste_x = int((LABEL_W * scale - barcode_img.width) / 2)
    paste_y = y * scale
    draw._image.paste(barcode_img, (paste_x, paste_y + 2 * scale))

# ---------------------------------------------------------------------------
def render_label(output_path, scale=SCALE):
    w, h = LABEL_W * scale, LABEL_H * scale
    img = Image.new("L", (w, h), 255)
    draw = ImageDraw.Draw(img)
    s = scale

    # Row 1 — 样本号
    draw_text(draw, 5, 5, "22", 28, s)

    # Row 2 — 组合项目
    draw_text(draw, 88, 8, "血常规（迈瑞流水线）", 18, s)

    # Row 3 — 条码
    draw_barcode_img(draw, 36, 72, "008085125", 75, 3, s)

    # Row 4 — 条码号
    draw_text(draw, 111, 152, "008085125", 18, s)

    # Row 5 — 姓名 / 标本 / 科室 (三栏)
    draw_text(draw, 5,   175, "廖明",     14, s)
    draw_text(draw, 145, 175, "全血",     13, s)
    draw_text(draw, 245, 175, "心血管内科二区", 13, s)

    # Row 6 — 病人号 / 日期 (双栏)
    draw_text(draw, 5,   205, "202629988",      16, s)
    draw_text(draw, 245, 205, "2026/5/15 9:24", 15, s)

    img.save(output_path)
    print(f"[preview] saved {w}×{h} px → {output_path}")
    if not CJK_FONT_PATH:
        print("[preview] WARNING: no CJK font found")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--scale",  type=int, default=SCALE)
    p.add_argument("--output", type=str, default="label_medical.png")
    args = p.parse_args()
    render_label(args.output, args.scale)

if __name__ == "__main__":
    main()
