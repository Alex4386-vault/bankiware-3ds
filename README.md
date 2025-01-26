<h1 align="center">Bankiware<br><small><sup><i>for Nintendo 3DS</i></sup></small></h1>


## Description
This is a homebrew application for running bankiware, __a game by [`paradot`](https://x.com/zenerat), released on touhou station game jam 2024__ on the Nintendo 3DS.

## Prerequisites
1. Download [Bankiware](https://para-dot.itch.io/bankiware) from the itch.io page.
2. Download **Unity for 3DS** from "your personal archive" or "use the search engine" to find it.
   - I will **NOT** provide a link to the Unity for 3DS download page., as it is against the [Nintendo's Developer NDA](https://developer.nintendo.com/group/development/nda) and is considered as a confidential information specified in [this notice (NDID login Required)](https://developer.nintendo.com/group/development/home/announcements?articleId=311774554).  
   - If you have signed up the NDID and signed NDA when Nintendo 3DS indie development was available, Good luck finding it!
3. Install [UndertaleModTool](https://github.com/UnderminersTeam/UndertaleModTool) to unpack the assets
4. Install [ffmpeg](https://ffmpeg.org/download.html) to convert the audio files to the correct format.

## Unpacking assets
1. Install `7z` or equivalent on your system to unpack the `Game.exe` executable (Right-Click, `Open Inside`).
2. Extract all of the contents into a folder.
3. Open `UndertaleModTool` and open the `data.win` file.
4. Run `Scripts` > `Resource Unpackers` > `ExportAllSounds.csx` to export all of the sounds. (If the script asks if you want to export `"external" ogg sounds`, click `No`)
5. Copy `Exported_Sounds/*.wav` and `*.ogg` files from the folder to `assets/audio` folder.
6. Run `Scripts` > `Resource Unpackers` > `ExportAllTexturesGrouped.csx` to export all of the Textures and Sprites.
7. Copy `Exported_Textures/*` folders to `assets/textures` folder.

## FAQ
1. **Why don't you provide unpacked resources?**
   - I am **NEVER** going to distribute the unpacked resources since it is incompliant to according to [Guidelines for Touhou Project Fan Creators (Last updated on 2020-11-10)](https://touhou-project.news/guidelines_en/) Article 2, "Anything that infringes upon other intellectual property.".  
     If the resource is lost, You have to recreate the resource by yourself, in doujin fashion! good luck!
