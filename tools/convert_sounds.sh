#!/bin/bash

# Create output directory if it doesn't exist
mkdir -p romfs/sounds

# Convert all wav and ogg files from raw/sounds to the required format
for input in raw/sounds/*.{wav,ogg}; do
    if [ -f "$input" ]; then
        filename=$(basename "$input")
        output="romfs/sounds/${filename%.*}.wav"
        
        # Convert to:
        # - 22050Hz sample rate (matches sound system)
        # - 16-bit PCM
        # - Stereo
        # - WAV format
        ffmpeg -i "$input" \
               -acodec pcm_s16le \
               -ac 2 \
               -ar 22050 \
               -y "$output"
        
        echo "Converted $input -> $output"
    fi
done

echo "Sound conversion complete!"