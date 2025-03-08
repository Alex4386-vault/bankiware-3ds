<h1 align="center">Bankiware<br><small><sup><i>for Nintendo 3DS</i></sup></small></h1>
<p align="center">Warioware-like touhou game on its home (Nintendo 3DS) at last!</p>

## Description
This is a homebrew application for running [Bankiware](https://para-dot.itch.io/bankiware), __a game by [`paradot`](https://x.com/zenerat), released on touhou station game jam 2024__ on the Nintendo 3DS.


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

## Preprocessing the Assets
1. Run `./copy.sh` to copy the assets to the correct location.

## Building the Project
By default, running `make` will:
1. Scale-down and add padding to images to the nearest power of 2 using `ImageMagick`.
   > **Why not pre-scale the images to the nearest power of 2?**
   > - As mentioned in FAQ, I am **TRYING** my best **NOT** to redistribute the original resources.
   > - The original resources are not in the power of 2, so I have to scale them down to the nearest power of 2 programatically, See `tools/texture_config.json` for scaling configs.
2. Convert the images to Nintendo's proprietary `t3x` format using `tex3ds` for Nintendo 3DS compatibility.
3. Convert the audio files to 22050Hz, 16-bit, mono, PCM WAV format using `ffmpeg` for Nintendo 3DS compatibility.
4. And last, build the project using `arm-none-eabi-gcc`.

If you are changing codebase rapidly, You can run `make codeonly` to build the C source code only without re-preprocessing the assets.

## Running the Project
There are multiple ways to run the project on **real hardware**:  
1. **Using a homebrew launcher** - For easy and quick way:
   - Copy the `bankiware.3dsx` file to the `/3ds` directory of your SD card.
   - Run the homebrew launcher via your favorite exploit and select `bankiware`.
2. **Build the `.cia` file** - For streamlined experience like e-shop games:
   1. Install [`bannertool` _(link broken)_](https://github.com/Steveice10/bannertool).  
   2. `./tools/create_banner.sh` 
   3. Run following command on the project root:
      ```bash
      bannertool makebanner -i title.png -a banner.wav -o bankiware.bnr
      ```
   4. Run following command on the project root:
      ```bash
      makerom -f cia -o bankiware.cia -DAPP_ENCRYPTED=false -rsf bankiware-3ds.rsf -target t -exefslogo -elf bankiware-3ds.elf -icon bankiware-3ds.smdh -banner bankiware.bnr
      ```
   5. Now copy `bankiware.cia` into your SD card and install it using FBI or any other CIA installer!

## FAQ
1. **Why don't you provide unpacked resources?**
   > I am **NEVER** going to distribute the unpacked resources since it is incompliant to according to [Guidelines for Touhou Project Fan Creators (Last updated on 2020-11-10)](https://touhou-project.news/guidelines_en/) Article 2, "Anything that infringes upon other intellectual property.".  
     If the resource is lost, You have to recreate the resource by yourself, in doujin fashion! good luck!
2. **The images displayed on the game is mushy**
   > Due to restrictions of **1**, I only have options for modifying images via "programmatically" that had been "extracted" by the end-user that have downloaded the game.  
   > By implementing this way <sub>(i.e. Spigot BuildTools method)</sub>, I can avoid the infringement of the original resources, and make sure that paradot gets well-deserved credit for building this fantastic game.  
   >   
   > Therefore, If you want to improve the image quality, modify the texture by yourself and update the conversion scripts, and coordinates on the source.
3. **Why are the coordinates of the images are off?**
   > See **2**.


## Disclaimer
This project is a fan-made project and is not in any way affiliated with the original creator (paradot) of the game.  

## License
For the code that I have wrote, I am releasing it under [UNLICENSE (Public domain)](https://unlicense.org/).  

For the `./src/include/bankiware_original.h` file, The name `bankiware`, and resources related to original game, Please refer to the original license of the game.  

