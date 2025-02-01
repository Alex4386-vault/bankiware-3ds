<h1 align="center">Bankiware<br><small><sup><i>for Nintendo 3DS</i></sup></small></h1>


## Description
This is a homebrew application for running bankiware, __a game by [`paradot`](https://x.com/zenerat), released on touhou station game jam 2024__ on the Nintendo 3DS.

## Prerequisites
1. Download [Bankiware](https://para-dot.itch.io/bankiware) from the itch.io page.
2. Install [UndertaleModTool](https://github.com/UnderminersTeam/UndertaleModTool) to unpack the assets
3. Download [DevkitPro](https://devkitpro.org/wiki/Getting_Started) to setup the Development Environment.
4. Install [ffmpeg](https://ffmpeg.org/download.html) to convert the audio files to the correct format.
   Supposing you are using Debian-based system:  
   ```bash
   sudo apt install ffmpeg
   ```
5. Install [ImageMagick](https://imagemagick.org/script/download.php), [jq](https://stedolan.github.io/jq/download/), [GNU Make](https://www.gnu.org/software/make/), [bc](https://www.gnu.org/software/bc/) to do the preprocessing of the assets.
   Supposing you are using Debian-based system:  
   ```bash
   sudo apt install imagemagick jq build-essential bc
   ```

## Unpacking assets
1. Install `7z` or equivalent on your system to unpack the `Game.exe` executable (Right-Click, `Open Inside`).
2. Extract all of the contents into a folder.
3. Open `UndertaleModTool` and open the `data.win` file.
4. Run `Scripts` > `Resource Unpackers` > `ExportAllSounds.csx` to export all of the sounds. (If the script asks if you want to export `"external" ogg sounds`, click `Yes`)
5. Copy `Exported_Sounds/`, `External_Sounds` folder to root of this repository.
6. Run `Scripts` > `Resource Unpackers` > `ExportAllTexturesGrouped.csx` to export all of the Textures and Sprites.
7. Copy `Exported_Textures/` folder to the root of this repository.


## FAQ
1. **Why don't you provide unpacked resources?**
   - I am **NEVER** going to distribute the unpacked resources since it is incompliant to according to [Guidelines for Touhou Project Fan Creators (Last updated on 2020-11-10)](https://touhou-project.news/guidelines_en/) Article 2, "Anything that infringes upon other intellectual property.".  
     If the resource is lost, You have to recreate the resource by yourself, in doujin fashion! good luck!
