#!/bin/bash

# Set parameters
SAMPLES=50
S1Y=20
S1Z=50
S2X=60
S2Y=30
S2Z=80
MAX_PARALLEL=2

# Function to generate one image
generate() {
  sleep $((RANDOM % 3))
  S1X=$1
  S1Y=$2
  S1Z=$3
  S2X=$4
  S2Y=$5
  S2Z=$6
  SAMPLES=$7

  OUTPUT_FILE="x_${S1X}.png"
  URL="http://localhost:8080/function/render/render?samples=${SAMPLES}&s1x=${S1X}&s1y=${S1Y}&s1z=${S1Z}&s2x=${S2X}&s2y=${S2Y}&s2z=${S2Z}"
  echo "Rendering s1x=$S1X -> $OUTPUT_FILE"
  curl -s --http1.1 --max-time 1000 "$URL" -o "$OUTPUT_FILE"
}

export -f generate

# Loop over s1x values 20–80 and invoke in parallel
seq 40 60 | xargs -P "$MAX_PARALLEL" -I {} bash -c \
  'generate "$@"' _ {} "$S1Y" "$S1Z" "$S2X" "$S2Y" "$S2Z" "$SAMPLES"
