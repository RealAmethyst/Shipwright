"""Validate native Options resource keys without loading the game."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[2]
issues = []

def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            issues.append(f"Duplicate resource: {key}")
        result[key] = value
    return result

bank = json.loads((root / "soh/assets/custom/accessibility/texts/options_eng.json").read_text(encoding="utf-8-sig"),
                  object_pairs_hook=unique_object)
for path in (root / "soh/soh").rglob("*.cpp"):
    source = path.read_text(encoding="utf-8-sig")
    if "NativeOptions" not in source and path.parent.name != "NativeOptions":
        continue
    keys = set(re.findall(r'(?:NativeOptions|N)::Text\("([^"]+)"\)', source))
    if path.parent.name == "NativeOptions":
        keys.update(re.findall(r'(?<![:\w])Text\("([^"]+)"\)', source))
    if path.name.startswith("NativeSaveEditor") or path.name == "debugSaveEditor.cpp":
        keys.update("save_" + key for key in re.findall(r'(?<![:\w])Text\("([^"]+)"\)', source))
    for key in sorted(keys):
        if key not in bank or not isinstance(bank[key], str) or not bank[key]:
            issues.append(f"{path.relative_to(root)}: missing text {key}")
if issues:
    raise SystemExit("\n".join(issues))
print("Native Options literal resource keys and duplicate-key checks passed.")
