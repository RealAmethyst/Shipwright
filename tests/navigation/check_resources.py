"""Check navigation's resource consumers against the actual text banks."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[2]
text_dir = root / "soh/assets/custom/accessibility/texts"
source_dir = root / "soh/soh/Enhancements/navigation"


def unique(pairs):
    result = {}
    for key, value in pairs:
        assert key not in result, f"Duplicate resource: {key}"
        result[key] = value
    return result


def bank(name):
    return json.loads((text_dir / (name + ".json")).read_text(encoding="utf-8-sig"), object_pairs_hook=unique)


targets = (source_dir / "NavigationTargets.cpp").read_text(encoding="utf-8-sig")
runtime = (source_dir / "Navigation.cpp").read_text(encoding="utf-8-sig")
navigation = bank("navigation_eng")
options = bank("options_eng")
keys = set(re.findall(r'(?<![\w:])(?:Text|Format)\("([^"]+)"', targets + runtime))
keys.update(re.findall(r'key = "([^"]+)"', targets))
for array in ("categories", "kinds"):
    body = re.search(rf'{array}\[\]\{{([^}}]+)\}}', targets).group(1)
    keys.update(("category_" if array == "categories" else "") + key for key in re.findall(r'"([^"]+)"', body))
keys.update(re.findall(r'"([^"]+)"', re.search(r'keys\[\]\{([^}]+)\}', runtime).group(1)))
for key in sorted(keys):
    assert isinstance(navigation.get(key), str) and navigation[key].strip(), f"Missing navigation text: {key}"
for key in re.findall(r'NativeOptions::Text\("([^"]+)"', targets):
    assert isinstance(options.get(key), str) and options[key].strip(), f"Missing port caption: {key}"

item_header = (root / "soh/include/z64item.h").read_text(encoding="utf-8-sig")
item_ids = {name: str(int(number, 16)) for number, name in
            re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(ITEM_\w+)\s*[,=]', item_header)}
items = set(re.findall(r'item = (ITEM_\w+)', targets)) - {"ITEM_NONE"}
for language in ("eng", "fra", "ger"):
    localized = bank("kaleidoscope_" + language)
    for item in sorted(items):
        assert item in item_ids, f"No native ID for {item}"
        value = localized.get(item_ids[item])
        assert isinstance(value, str) and value.strip(), f"Missing {language} caption for {item}"
print(f"Navigation resources passed: {len(keys)} menu/target keys, port captions, and {len(items)} native item IDs in three languages.")
