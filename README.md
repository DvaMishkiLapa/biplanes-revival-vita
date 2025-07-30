# Biplanes Revival for PS Vita

An old cellphone arcade "BlueTooth BiPlanes"
recreated for PC (originally developed by Morpheme Ltd. in 2004).

- [PLAY IN BROWSER](https://itch.io/embed-upload/13252895?color=009AEF)

- [Biplanes Revival for PS Vita](#biplanes-revival-for-ps-vita)
  - [Gameplay videos](#gameplay-videos)
  - [Download the game](#download-the-game)
  - [Features](#features)
  - [Modding](#modding)
  - [Building](#building)
  - [Thanks and Credits](#thanks-and-credits)
    - [My Thanks](#my-thanks)
    - [Thanks to the authors of the original port](#thanks-to-the-authors-of-the-original-port)

## Gameplay videos

[Singleplayer gameplay](https://github.com/regular-dev/biplanes-revival/assets/67646403/4f7d6371-6c9f-4271-a6c7-d17902a5ed2f)

- [Singleplayer gameplay pt 1](https://youtu.be/FYtIZ7ptaSo)
- [Singleplayer gameplay pt 2](https://youtu.be/4pWHn85Ez0o)
- [Multiplayer gameplay (legacy version)](https://youtu.be/mIgMNh6gGXs)

## Download the game

- VitaDB (soon);
- From this repository's [Releases page](https://github.com/DvaMishkiLapa/biplanes-revival-vita/releases);

Custom soundpacks are available
on our [website](https://regular-dev.org/biplanes-revival)
and on our [itch.io page](https://regular-dev.itch.io/biplanes-revival)

## Features

- Easy to learn, hard to master gameplay;
- Immerse yourself in quick and intense dogfights;
- Shoot & dodge, bail out & respawn to outsmart your rival;
- Challenging AI with 4 difficulty levels;
- Easy peer-to-peer matchmaking with private sessions support;
- Optional gameplay modifiers:
  - one-shot kills;
  - extra clouds for cover;
  - alternative hitboxes for challenge;
- Verbose statistics system;
- Moddable sounds & sprites.

Written in C++ from scratch using reworked sprites from the original game.

Graphics: SDL2

Netcode based on Simple Network Library from "Networking for Game Programmers" by Glenn Fiedler.

If you're interested in history of this project, you can read [original port devlog](https://regular-dev.itch.io/biplanes-revival/devlog/714967/5th-year-anniversary-update).

## Modding

You can replace the files in the `assets` folder in the `.vpk` file.

## Building

1. Make sure that you have [`cmake`](https://cmake.org) installed;
2. Make sure you have [VitaSDK](https://vitasdk.org) installed and configured (try [vdpm](https://github.com/vitasdk/vdpm));
3. Build the project with the following commands:

  ```bash
  cmake -S . -B build
  cmake --build build
  ```

Then, install the generated `./build/BiplanesRevival.vpk` file on your PS Vita.

## Thanks and Credits

### My Thanks

- Thank you, [regular-dev](https://github.com/regular-dev), for playing this game as I did when I was a child. And for finding the strength to revive it;
- Thank you to all the contributors to [VitaSDK](https://vitasdk.org), [Vitadev Package manager](https://github.com/vitasdk/vdpm);
- [Vita Nuova](https://discord.com/invite/PyCaBx9) and [HENkaku](https://discord.com/invite/m7MwpKA) Discord servers;
- [gl33ntwine](https://github.com/v-atamanenko) for creating [the awesome article](https://gl33ntwine.com/posts/develop-for-vita).

### Thanks to the authors of the original port

- Linux port, matchmaking and AI would never come to life without [xion's](https://github.com/xxxxxion) help. Kudos to this person!
- Big thanks to [punchingdig](https://www.youtube.com/user/punchingdig)
  for his artistic help (upscaling the splash screen,
  making custom soundpacks & cozy Christmas theme, redrawing some of the sprites & frames, etc.).
  Not to mention countless hours of patient playtesting...
