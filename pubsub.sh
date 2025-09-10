#!/bin/bash

# Configuration
TOPIC_ID="eventarc-us-central1-trigger-oqsebsrp-758"
NUM_MESSAGES=1000000
CONCURRENCY=100 # Adjust this value to control the number of parallel processes

# --- DO NOT EDIT BELOW THIS LINE ---

echo "Starting stress test for topic: $TOPIC_ID"
echo "Publishing $NUM_MESSAGES messages with a concurrency of $CONCURRENCY."

start_time=$(date +%s)

# Create a sequence of messages and pipe them to xargs for parallel processing
seq 1 "$NUM_MESSAGES" | xargs -P "$CONCURRENCY" -I {} gcloud pubsub topics publish "$TOPIC_ID" --message="Thorfinn_{}"

end_time=$(date +%s)
duration=$((end_time - start_time))

echo "------------------------------------------------------"
echo "Stress test complete!"
echo "Published $NUM_MESSAGES messages in $duration seconds."
