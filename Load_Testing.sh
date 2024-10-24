#!/bin/bash

# URL to which requests will be sent
URL="http://localhost:8080/"  # Replace with your target URL

# Temp file to store the response codes
RESPONSE_FILE=$(mktemp)

# Function to make a request and capture the HTTP status code
make_request() {
  STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$URL")
  
  # Write status code to the file in a thread-safe manner
  echo "$STATUS" >> "$RESPONSE_FILE"
}

# Loop to make 1000 concurrent requests
for ((i=1;i<=1000;i++)); do
  make_request &
done

# Wait for all background processes to finish
wait

# Ensure all writes are completed, and then count successful responses (HTTP 200)
SUCCESS_COUNT=$(grep -c "^200$" "$RESPONSE_FILE")

# Output the number of successful responses
echo "Number of successful responses: $SUCCESS_COUNT"

# Cleanup the temporary file
rm "$RESPONSE_FILE"
