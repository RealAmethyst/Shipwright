"""Render the production menu layout with the user's extracted font and panel.

This standalone check does not initialize or launch the game. ROM-derived assets
stay in the supplied archive; previews are local verification artifacts.
"""

import argparse
import json
import re
import struct
import subprocess
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw


def texture(archive, name):
    data = archive.read(name)
    version = struct.unpack_from("<I", data, 8)[0]
    kind, width, height = struct.unpack_from("<III", data, 64)
    offset = 80 if version == 0 else 92
    payload = data[offset:]
    pixels = []
    if kind == 1:
        assert len(payload) == width * height * 4
        pixels = list(zip(payload[::4], payload[1::4], payload[2::4], payload[3::4]))
    elif kind == 2:
        assert len(payload) == width * height * 2
        for high, low in zip(payload[::2], payload[1::2]):
            value = (high << 8) | low
            pixels.append((((value >> 11) & 31) * 255 // 31, ((value >> 6) & 31) * 255 // 31,
                           ((value >> 1) & 31) * 255 // 31, (value & 1) * 255))
    elif kind == 9:
        assert len(payload) == width * height * 2
        for intensity, alpha in zip(payload[::2], payload[1::2]):
            pixels.append((intensity, intensity, intensity, alpha))
    elif kind == 5:
        assert len(payload) == width * height // 2
        for byte in payload:
            for nibble in (byte >> 4, byte & 15):
                intensity = nibble * 17
                pixels.append((255, 255, 255, intensity))
    elif kind == 6:
        assert len(payload) == width * height
        pixels = [(255, 255, 255, value) for value in payload]
    elif kind == 8:
        assert len(payload) == width * height
        pixels = [((value >> 4) * 17,) * 3 + ((value & 15) * 17,) for value in payload]
    else:
        raise ValueError(f"Unsupported preview texture type {kind}: {name}")
    image = Image.new("RGBA", (width, height))
    image.putdata(pixels)
    return image


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--checks", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    args.output.mkdir(parents=True, exist_ok=True)
    message = (root / "soh/src/code/z_message_PAL.c").read_text(encoding="utf-8")
    widths_body = message.split("f32 sFontWidths[144] = {", 1)[1].split("};", 1)[0]
    widths = [float(value) for value in re.findall(r"^\s*([\d.]+)f,", widths_body, re.M)]
    assert len(widths) == 144
    font_source = (root / "soh/src/code/z_kanfont.c").read_text(encoding="utf-8")
    font_names = re.findall(r"gMsg\w+Tex", font_source.split("fontTbl[140] = {", 1)[1].split("};", 1)[0])
    assert len(font_names) == 140
    font_paths = {}
    for bank in ("nes_font_static", "message_static"):
        header = (root / f"soh/assets/textures/{bank}/{bank}.h").read_text()
        font_paths.update(re.findall(r'#define d(gMsg\w+Tex) "__OTR__([^"]+)"', header))
    resources = json.loads((root / "soh/assets/custom/accessibility/texts/options_eng.json").read_text())
    samples = {
        "categories": {
            "title": resources["title"],
            "rows": [{"id": key, "label": resources[key]} for key in (
                "search", "accessibility", "audio", "display", "controls", "gameplay", "cosmetics",
                "randomizer", "trackers", "network", "system", "advanced")],
        },
        "audio": {
            "title": resources["audio"],
            "focus": "main",
            "rows": [
                {"id": "master", "label": "Master Volume", "value": "40%"},
                {"id": "main", "label": "Main Music Volume", "value": "100%"},
                {"id": "sub", "label": "Sub Music Volume", "value": "100%"},
                {"id": "fanfare", "label": "Fanfare Volume", "value": "100%"},
                {"id": "sfx", "label": "Sound Effects Volume", "value": "100%"},
                {"id": "backend", "label": "Audio API (Needs reload)", "value": "Windows Audio Session API"},
            ],
        },
        "long-description": {
            "title": "Audio Options",
            "rows": [
                {"id": "music", "label": "Disable Leading Music in Lost Woods", "value": "Off",
                 "description": "Disables the volume shifting in the Lost Woods. Useful for hearing your custom music in the Lost Woods if you don't need the navigation assistance the volume changing provides. If toggling this while in the Lost Woods, reload the area for the effect to kick in."},
                {"id": "random", "label": "Automatically Randomize All Music and Sound Effects", "value": "On File Load (Seeded)"},
                {"id": "octave", "label": "Lower Octaves of Unplayable High Notes", "value": "On"},
            ],
        },
    }
    bow = {"resource": "textures/icon_item_static/gItemIconBowTex", "width": 32, "height": 32}
    nut = {"resource": "textures/icon_item_static/gItemIconDekuNutTex", "width": 32, "height": 32}
    samples["hash-icons"] = {
        "title": resources["plando_hash"],
        "rows": [{"id": str(i), "label": f"Slot {i + 1}", "value": "Fairy Bow" if i % 2 else "Deku Nut",
                  "image": bow if i % 2 else nut} for i in range(5)],
    }
    samples["texture-preview"] = {
        "title": resources["gfx_loaded_texture_0"], "description": "32 x 32, RGBA32", "popup": True,
        "image": bow, "rows": [{"id": "close", "label": resources["close"]}],
    }
    samples["confirmation"] = {
        "title": resources["delete"], "description": "Example split list", "popup": True,
        "rows": [{"id": "cancel", "label": resources["cancel"]}, {"id": "accept", "label": resources["delete"]}],
    }
    samples["details"] = {
        "title": "Disable Leading Music in Lost Woods",
        "documentLines": [
            "Disables the volume shifting in the Lost Woods.",
            "Useful for hearing your custom music in the",
            "Lost Woods if you don't need the navigation",
            "assistance the volume changing provides.",
            "", "If toggling this while in the Lost Woods,",
            "reload the area for the effect to kick in."],
        "rows": [{"id": "close", "label": resources["close"]}], "footer": resources["document_hints"],
    }
    samples["pause-options"] = {
        "title": resources["title"],
        "rows": [{"id": "open", "label": resources["pause_open_options"],
                  "description": resources["pause_options_help"]}],
        "footer": resources["pause_options_z_hints"],
    }
    samples["controller-capture"] = {
        "title": resources["bind_capture"], "description": resources["bind_capture_help"], "popup": True,
        "rows": [{"id": "waiting", "label": resources["bind_waiting"]}],
        "footer": resources["bind_capture_hint"],
    }
    for selection in (0, 1):
        samples[f"title-{selection}"] = {
            "title": "", "rows": [], "titleMenu": True, "selection": selection, "start": "PRESS START",
        }
    with zipfile.ZipFile(args.archive) as archive:
        # The final controller-pad glyph is optional in this port. Load only glyphs
        # present in the archive, and fail if the production layout requests one
        # that cannot be resolved; an unused optional glyph does not affect QA.
        fonts = {i + 32: texture(archive, font_paths[name]) for i, name in enumerate(font_names)
                 if font_paths[name] in archive.namelist()}
        panels = [texture(archive, f"textures/title_static/gFileSelWindow{i}Tex") for i in range(1, 21)]
        images = {info["resource"]: texture(archive, info["resource"]) for info in (bow, nut)}
        title_art = [
            (texture(archive, "objects/object_mag/gTitleZeldaShieldLogoTex"), 72, 20),
            (texture(archive, "objects/object_mag/gTitleTheLegendOfTextTex"), 145, 72),
            (texture(archive, "objects/object_mag/gTitleOcarinaOfTimeTMTextTex"), 143, 126),
            (texture(archive, "objects/object_mag/gTitleCopyright1998Tex"), 94, 198),
        ]
    for name, sample in samples.items():
        sample["widths"] = widths
        command_path = args.output / f"{name}.json"
        subprocess.run([str(args.checks), "--preview", str(command_path)],
                       input=json.dumps(sample), text=True, check=True)
        commands = json.loads(command_path.read_text())
        for screen_width, suffix in ((320, ""), (426, "-wide")):
            scale = 4
            image = Image.new("RGBA", (screen_width * scale, 240 * scale), (3, 8, 21, 255))
            x_offset = (screen_width - 320) / 2
            if sample.get("titleMenu"):
                # Static art placement from EnMag_DrawInner. The animated world
                # and flame effect are deliberately outside this layout check.
                for source, left, top in title_art:
                    image.alpha_composite(source.resize((source.width * scale, source.height * scale),
                                                        Image.Resampling.BILINEAR),
                                          (round((left + x_offset) * scale), top * scale))
            for command in commands:
                x, y = round((command["x"] + x_offset) * scale), round(command["y"] * scale)
                w, h = round(command["width"] * scale), round(command["height"] * scale)
                color = tuple(command["color"])
                if command["type"] == 0:
                    layer = Image.new("RGBA", image.size)
                    ImageDraw.Draw(layer).rectangle((x, y, x + w - 1, y + h - 1), fill=color)
                    image = Image.alpha_composite(image, layer)
                    continue
                if command["type"] == 1:
                    source = fonts[command["texture"]]
                elif command["type"] == 2:
                    source = panels[command["texture"]]
                else:
                    source = images[command["resource"]]
                tinted = Image.new("RGBA", source.size)
                tinted.putdata([tuple(pixel[i] * color[i] // 255 for i in range(4)) for pixel in source.getdata()])
                tinted = tinted.resize((w, h), Image.Resampling.BILINEAR)
                image.alpha_composite(tinted, (x, y))
            image.convert("RGB").save(args.output / f"{name}{suffix}.png")
    print(f"Rendered {len(samples) * 2} native Options previews to {args.output}")


if __name__ == "__main__":
    main()
