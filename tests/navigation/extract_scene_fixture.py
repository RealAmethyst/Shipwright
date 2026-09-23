"""Read the user's archive for offline collision checks; never package the output."""
import argparse
import hashlib
from pathlib import Path
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("archive", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
resource = "scenes/shared/spot04_scene/spot04_sceneCollisionHeader_008918"
with zipfile.ZipFile(args.archive) as archive:
    collision = archive.read(resource)
with args.output.open("xb") as output:
    output.write(collision)
print(f"Kokiri collision SHA-256: {hashlib.sha256(collision).hexdigest()}")
