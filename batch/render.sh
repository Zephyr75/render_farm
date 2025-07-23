#!/bin/bash

SAMPLES=5
S1Y=20
S1Z=50
S2X=60
S2Y=30
S2Z=80
MAX_PARALLEL=3

generate() {
  S1X=$1
  OUTPUT_FILE="x_${S1X}.png"
  URL="http://localhost:8080/function/render/render?samples=${SAMPLES}&s1x=${S1X}&s1y=${S1Y}&s1z=${S1Z}&s2x=${S2X}&s2y=${S2Y}&s2z=${S2Z}"
  echo "Rendering s1x=$S1X -> $OUTPUT_FILE"
  curl -s --http1.1 "$URL" -o "$OUTPUT_FILE"
}

export -f generate

# Use xargs to run up to MAX_PARALLEL jobs at once
seq 40 60 | xargs -P $MAX_PARALLEL -I {} bash -c 'generate "$@"' _ {}
