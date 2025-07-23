#!/bin/bash

# Fixed parameters
SAMPLES=100
S1Y=20
S1Z=50
S2X=60
S2Y=30
S2Z=80

# Loop over s1x values from 20 to 80
for S1X in {20..80}; do
    OUTPUT_FILE="x_${S1X}.png"
    URL="http://localhost:8080/function/render/render?samples=${SAMPLES}&s1x=${S1X}&s1y=${S1Y}&s1z=${S1Z}&s2x=${S2X}&s2y=${S2Y}&s2z=${S2Z}"
    echo "Rendering image for s1x=${S1X} -> ${OUTPUT_FILE}"
    curl -s "$URL" -o "$OUTPUT_FILE"
done
