#!/bin/bash

OUTPUT=src/gamecontrollerdb

ensure-valid-id () {
  grep -E '^[0-9a-f]{32},'
}

footer () {
  printf "\n" >> $OUTPUT
}



#Cleanup
rm $OUTPUT

# Add the SDL DB header
printf "# Source: https://github.com/SDL-mirror/SDL/blob/master/src/joystick/SDL_gamecontrollerdb.h\n\n" >> $OUTPUT

# Add the SDL DB
curl https://raw.githubusercontent.com/SDL-mirror/SDL/master/src/joystick/SDL_gamecontrollerdb.h \
  | awk '/LINUX/{flag=1;next}/endif/{flag=0}flag' \
  | sed -n 's/.*"\(.*\)".*/\1/p' \
  | ensure-valid-id | sort >> $OUTPUT

footer

# Add the GameControllerDB header
printf "# Source: https://github.com/gabomdq/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt\n\n" >> $OUTPUT

# Add the GameControllerDB
curl https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt \
  | grep "platform:Linux" \
  | sed 's/platform:Linux,//' \
  | ensure-valid-id | sort >> $OUTPUT

footer

