import glob
import os
headers = glob.glob("*.h")
sources = glob.glob("*.c")
buildScript = "gcc -Wall -g"

for h in headers:
    buildScript = buildScript + " " + h + " "
for c in sources:
    buildScript = buildScript + " " + c + " "

buildScript = buildScript + " -o build/N3D -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -lfluidsynth -lm -D NEW_SPRITES"

os.system(buildScript)