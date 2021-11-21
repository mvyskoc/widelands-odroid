# Widelands
The Widelands is computer game similar to the game Settlers.
This project is fork of the original game https://github.com/widelands/widelands.git
It is based on the old release version 15.1 and it tries port the game for ARM devices specifically for Odroid GO Super.

The software release 15.1 was the last release without Open GL. Affraid this release use SDL1.2 library which cannot be used without X-Server.

Fortunately dependency on SDL1.2 library was replaced with [sdl12-compat](https://github.com/libsdl-org/sdl12-compat.git) library, which use as backend SDL2.


This project is usefull for owners of ARM devices such as: Odroid, Raspberry PI, etc.
  
If you have computer with Open GL or you are windows user  look at the original project website and download current release. This project has nothing what can offer to you. With compare to the original game I am not capable to implement new game mechanism.

# Modifications
  - Add compatibility with new libpng library (tested with 1.6). Without this bugfix the game was not possible compile on the current system.

  - Backported bugfix for [Bug 905930](https://bugs.launchpad.net/widelands/+bug/905930)
  The game crashed at the start of the campaing with error: `Graphic::save_png: could not set png setjmp`

  - Bugfix with loading locales. The the internal locale names were renamed, backported FileSystem functions for better support of relative paths.

  - Remove SDL1.2 libraray and replace it with static [sdl12-compat](https://github.com/libsdl-org/sdl12-compat.git)

# TODO
 - I will add configuration files for XboxDrv driver for Joystick / gamepad support. 
 - Start scripts and compiled binary files for Odroid Go Super

# Compile
The game can be compiled directly on the ARM device or in chroot.


Clone repositiory with its submodules in `libext` folder
````
git clone --recurse-submodules https://github.com/mvyskoc/widelands-odroid.git --single-branch
````

For compilation install dependency files, I hope I did not forget something
````
sudo apt install lua5.1 liblua5.1-0-dev liboggz2-dev libsdl-net1.2-dev libsdl2-net-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev  libsdl-gfx1.2-dev gettext
````

Start build process
````
build_and_run.sh
````
