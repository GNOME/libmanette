#!/bin/bash

OUTPUT=src/gamecontrollerdb

ensure-valid-id () {
  grep -E '^[0-9a-f]{32},'
}

filter_use_button_labels_hint () {
  grep -v ',hint:SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1' | \
  sed 's|hint:!SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1,||g'
}

filter_platform () {
  grep "platform:Linux" | \
  sed 's|platform:Linux,||g'
}

filter_crc () {
  grep -v ',crc:'
}

footer () {
  printf "\n" >> $OUTPUT
}



#Cleanup
rm $OUTPUT

# Add the SDL DB header
printf "# Source: https://raw.githubusercontent.com/libsdl-org/SDL/refs/heads/main/src/joystick/SDL_gamepad_db.h\n\n" >> $OUTPUT

# Add the SDL DB
curl https://raw.githubusercontent.com/libsdl-org/SDL/refs/heads/main/src/joystick/SDL_gamepad_db.h \
  | awk '/LINUX/{flag=1;next}/endif/{flag=0}flag' \
  | sed -n 's/.*"\(.*\)".*/\1/p' \
  | filter_use_button_labels_hint \
  | filter_crc \
  | ensure-valid-id | sort >> $OUTPUT

footer

# Add the GameControllerDB header
printf "# Source: https://github.com/gabomdq/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt\n\n" >> $OUTPUT

# Add the GameControllerDB
curl https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt \
  | filter_platform \
  | ensure-valid-id | sort >> $OUTPUT

footer

