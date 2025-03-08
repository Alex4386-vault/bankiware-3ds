#!/bin/bash

# Create necessary directories
mkdir -p raw/sounds
mkdir -p raw/img

# Function to copy specific files
copy_specific_files() {
    local src="$1"
    local dest="$2"
    local type="$3"
    
    if [ ! -d "$src" ]; then
        echo "Warning: Source directory $src not found, skipping..."
        return 1
    fi
    
    echo "Copying $type from $src..."
    
    case "$type" in
        "sounds")
            # Define specific sound patterns to copy
            patterns=(
                "bgm_bossgame2" "bgm_end" "bgm_gameover" "bgm_gameover2"
                "bgm_jingle1" "bgm_jingle2" "bgm_jingleBossClear" "bgm_jingleBossFailed"
                "bgm_jingleBossstage" "bgm_jingleNext" "bgm_jingleSpeedUp"
                "bgm_microgame1" "bgm_microgame2" "bgm_ready" "bgm_test"
                "se_beam" "se_boyon2" "se_cursor" "se_decide" "se_eat"
                "se_huseikai" "se_nyu2" "se_pa3" "se_poyon1" "se_poyon2"
                "se_rappa" "se_seikai"
            )
            
            # Copy each required sound file
            for pattern in "${patterns[@]}"; do
                found_file=$(find "$src" -type f \( -name "${pattern}.wav" -o -name "${pattern}.ogg" \) -print -quit)
                if [ -n "$found_file" ]; then
                    cp "$found_file" "$dest/"
                    echo "Copied: $found_file"
                fi
            done
            ;;
            
        "textures")
            # Copy from Backgrounds directory
            if [ -d "$src/Backgrounds" ]; then
                find "$src/Backgrounds" -type f -name "*.png" -exec cp {} "$dest/" \;
            fi

            # Copy from Sprites directory
            if [ -d "$src/Sprites" ]; then
                find "$src/Sprites" -type f -name "*.png" -exec cp {} "$dest/" \;
            fi
            ;;
        *)
            echo "Unknown type: $type"
            return 1
            ;;
    esac
}

# Copy required sounds from Exported_Sounds and External_Sounds
copy_specific_files "Exported_Sounds" "raw/sounds" "sounds"
copy_specific_files "External_Sounds" "raw/sounds" "sounds"

# Copy textures from Export_Textures
copy_specific_files "Export_Textures" "raw/img" "textures"

echo "File copying complete! "

# Make the conversion scripts executable
chmod +x tools/convert_sounds.sh
chmod +x tools/convert_textures.sh