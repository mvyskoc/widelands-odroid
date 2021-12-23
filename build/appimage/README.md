Project website https://github.com/mvyskoc/widelands-odroid
If you find some issue let me know either via github or by
email: m.vyskoc@seznam.cz


The AARCH64 version was tested with handheld device Odroid GO Super
on the following system:
  - TECHTOYTINKER The Retro Arena version 4 and 5.2.1
   https://techtoytinker.com/odroid-go-super%2Fadv
    - TheRA-OGS-OGA-RC4.rar
    - TheRA-OGS-OGA-RC5.2.1-NTFS.rar

Start the game with bash script ./Widelands.sh

On the Retro Arena copy the folder Widelands into the folder /roms

For change locale is neccessary to generate locale for your language with the command
    sudo dpkg-reconfigure locales
    
For gamepad is neccessary to have write permission to /dev/urandom
This can be automaticaly fixed by adding udev rules by following command
    ./Widelands.sh


Known issues
============

TheRA-OGS-OGA-RC5.2.1-NTFS
--------------------------
At least in my case the device frozen, it is very lazy. The game is not
totaly executed event the home and config folders are note created.
If you reformat /roms partition to ext4 the problems are solved. Currently
I do not know why there is problem with NTFS partition.

