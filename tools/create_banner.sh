#!/bin/bash
mkdir -p banner

# Detect ImageMagick version and set command format
if command -v magick &> /dev/null; then
    version=$(magick -version | grep -oE "Version: ImageMagick [0-9]+" | grep -oE "[0-9]+")
    if [ "$version" -ge 7 ]; then
        echo "Using ImageMagick v7+ syntax"
        CONVERT="magick"
    else
        echo "Using ImageMagick v6 syntax"
        CONVERT="magick convert"
    fi
else
    echo "Using legacy ImageMagick syntax"
    CONVERT="convert"
fi

# banner.wav
ffmpeg -i "raw/sounds/bgm_jingle1.wav" -acodec pcm_s16le -ac 2 -ar 22050 -y "banner/banner.wav"

# resize to 256x128 with padding up and down for retaining aspect ratio and margin being transparent via imagemagick
$CONVERT "raw/img/spr_title_0.png" -resize 256x128 -background transparent -gravity center -extent 256x128 "banner/banner.png"
