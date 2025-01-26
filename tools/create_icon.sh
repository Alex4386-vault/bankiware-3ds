#!/bin/bash

# Create a simple 48x48 icon with the letter "B" for Bankiware
magick convert -size 48x48 xc:transparent \
    -fill "#4169E1" -draw "circle 24,24 24,2" \
    -fill white -font Arial -pointsize 30 -gravity center -draw "text 0,0 'B'" \
    icon.png