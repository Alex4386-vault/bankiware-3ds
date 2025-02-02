#!/bin/bash

# Function to get next power of 2 (max 1024)
get_next_power_of_2() {
    local n=$1
    n=$(($n-1))
    n=$(($n|$n>>1))
    n=$(($n|$n>>2))
    n=$(($n|$n>>4))
    n=$(($n|$n>>8))
    n=$(($n|$n>>16))
    n=$(($n+1))
    # Cap at 1024 for 3DS limitation
    if [ $n -gt 1024 ]; then
        n=1024
    fi
    echo $n
}

# Check if jq is installed
if ! command -v jq &> /dev/null; then
    echo "Error: jq is required but not installed. Please install jq first."
    exit 1
fi

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

# Create necessary directories
mkdir -p data/textures
mkdir -p temp_textures

# Load configuration
CONFIG_FILE="tools/texture_config.json"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Warning: Configuration file not found, using defaults"
    DEFAULT_MAX_WIDTH=256
    DEFAULT_MAX_HEIGHT=256
    DEFAULT_ALIGNMENT="center"
    DEFAULT_ROTATION=0
else
    DEFAULT_MAX_WIDTH=$(jq -r '.defaults.maxWidth // 256' "$CONFIG_FILE")
    DEFAULT_MAX_HEIGHT=$(jq -r '.defaults.maxHeight // 256' "$CONFIG_FILE")
    DEFAULT_ALIGNMENT=$(jq -r '.defaults.alignment // "center"' "$CONFIG_FILE")
    DEFAULT_ROTATION=$(jq -r '.defaults.rotation // 0' "$CONFIG_FILE")
fi

# Convert all PNG files from raw/img directory
echo "Converting textures..."
for img in raw/img/*.png; do
    if [ -f "$img" ]; then
        filename=$(basename "$img" .png)
        
        echo "Processing $filename..."
        # Get image dimensions and details
        dimensions=$($CONVERT "$img" -format "Size: %wx%h\nAspect: %[fx:w/h]\nDepth: %z-bit\nColorspace: %[colorspace]" info:)
        echo "Original image details:\n$dimensions"
        
        width=$($CONVERT "$img" -format "%w" info:)
        height=$($CONVERT "$img" -format "%h" info:)
        
        if [ -z "$width" ] || [ -z "$height" ]; then
            echo "Error: Could not get dimensions for $img"
            continue
        fi
        
        echo "Original aspect ratio: $(bc -l <<< "scale=4; $width/$height")"

        # Check if custom dimensions and alignment are specified in config
        if [ -f "$CONFIG_FILE" ]; then
            custom_width=$(jq -r ".textures.\"$filename\".width // 0" "$CONFIG_FILE")
            custom_height=$(jq -r ".textures.\"$filename\".height // 0" "$CONFIG_FILE")
            alignment=$(jq -r ".textures.\"$filename\".alignment // .defaults.alignment // \"center\"" "$CONFIG_FILE")
            scale_ratio=$(jq -r ".textures.\"$filename\".scaleRatio // \"\"" "$CONFIG_FILE")
            rotation=$(jq -r ".textures.\"$filename\".rotation // .defaults.rotation // 0" "$CONFIG_FILE")
            
            if [ "$custom_width" != "0" ] && [ "$custom_height" != "0" ]; then
                echo "Using custom dimensions for $filename: ${custom_width}x${custom_height}"
                target_width=$custom_width
                target_height=$custom_height
            else
                target_width=$DEFAULT_MAX_WIDTH
                target_height=$DEFAULT_MAX_HEIGHT
            fi
        else
            target_width=$DEFAULT_MAX_WIDTH
            target_height=$DEFAULT_MAX_HEIGHT
            alignment=$DEFAULT_ALIGNMENT
            scale_ratio=""
            rotation=$DEFAULT_ROTATION
        fi

        # Convert alignment to ImageMagick gravity
        case "$alignment" in
            "left")
                gravity="West"
                ;;
            "right")
                gravity="East"
                ;;
            "top")
                gravity="North"
                ;;
            "bottom")
                gravity="South"
                ;;
            *)
                gravity="Center"
                ;;
        esac
        echo "Using alignment: $alignment (gravity: $gravity)"

        # Handle scaling based on scaleRatio if specified
        if [ "$scale_ratio" = "height" ]; then
            echo "Using height-based scaling"
            # Scale to target height while maintaining aspect ratio
            $CONVERT "$img" -resize x${target_height} "temp_textures/${filename}_resized.png"
            # Place in target canvas with specified alignment
            $CONVERT "temp_textures/${filename}_resized.png" -background none -gravity $gravity -extent ${target_width}x${target_height} "temp_textures/${filename}.png"
        elif [ "$scale_ratio" = "width" ]; then
            echo "Using width-based scaling"
            # Scale to target width while maintaining aspect ratio
            $CONVERT "$img" -resize ${target_width} "temp_textures/${filename}_resized.png"
            # Place in target canvas with specified alignment
            $CONVERT "temp_textures/${filename}_resized.png" -background none -gravity $gravity -extent ${target_width}x${target_height} "temp_textures/${filename}.png"
        else
            echo "Using default scaling behavior"
            # Default behavior: scale proportionally to fit within target dimensions
            if [ $width -gt $target_width ] || [ $height -gt $target_height ]; then
                # Calculate scaling ratios
                width_ratio=$(bc -l <<< "scale=4; $target_width / $width")
                height_ratio=$(bc -l <<< "scale=4; $target_height / $height")
                
                # Use the smaller ratio to ensure entire image fits
                if (( $(bc -l <<< "$width_ratio < $height_ratio") )); then
                    scale_ratio=$width_ratio
                else
                    scale_ratio=$height_ratio
                fi
                
                # Calculate new dimensions
                new_width=$(bc <<< "scale=0; ($width * $scale_ratio + 0.5)/1")
                new_height=$(bc <<< "scale=0; ($height * $scale_ratio + 0.5)/1")
                
                echo "Scaling to fit within target: ${new_width}x${new_height}"
                $CONVERT "$img" -resize ${new_width}x${new_height} "temp_textures/${filename}.png"
            else
                # If image is smaller than target, use as is
                cp "$img" "temp_textures/${filename}.png"
            fi
        fi

        # Apply rotation if specified
        if [ $rotation -ne 0 ]; then
            echo "Applying rotation: ${rotation} degrees"
            $CONVERT "temp_textures/${filename}.png" -rotate ${rotation} "temp_textures/${filename}_rotated.png"
            mv "temp_textures/${filename}_rotated.png" "temp_textures/${filename}.png"
        fi

        # Get actual dimensions after rotation
        current_width=$($CONVERT "temp_textures/${filename}.png" -format "%w" info:)
        current_height=$($CONVERT "temp_textures/${filename}.png" -format "%h" info:)

        # Calculate power of 2 dimensions based on actual image size
        new_width=$(get_next_power_of_2 $current_width)
        new_height=$(get_next_power_of_2 $current_height)
        
        echo "Converting $filename: Original(${width}x${height}) -> Current(${current_width}x${current_height}) -> Power2(${new_width}x${new_height})"
        
        # Pad to power of 2 dimensions using specified alignment
        echo "Padding to power of 2: ${new_width}x${new_height} with alignment: $alignment"
        $CONVERT "temp_textures/${filename}.png" -background none -gravity $gravity -extent ${new_width}x${new_height} "temp_textures/${filename}_padded.png"
        echo "After padding: $($CONVERT "temp_textures/${filename}_padded.png" -format "%wx%h" info:)"
        
        # Save final PNG before t3x conversion
        cp "temp_textures/${filename}_padded.png" "temp_textures/${filename}.png"
        
        if [ ! -f "temp_textures/${filename}.png" ]; then
            echo "Error: Failed to process $img"
            continue
        fi
        
        echo "Final PNG dimensions before t3x conversion: $($CONVERT "temp_textures/${filename}.png" -format "%wx%h" info:)"
        # Convert to t3x
        echo "Converting to t3x"
        tex3ds -f rgba8 -z auto "temp_textures/${filename}.png" -o "data/textures/${filename}.t3x"
        
        if [ ! -f "data/textures/${filename}.t3x" ]; then
            echo "Error: Failed to convert to t3x format"
            continue
        fi
        
        # Print t3x file size for verification
        t3x_size=$(stat -f%z "data/textures/${filename}.t3x")
        echo "Generated t3x file size: ${t3x_size} bytes"
    fi
done

# Clean up temporary files
#rm -rf temp_textures

echo "Texture conversion complete"