#!/bin/sh
# usage: run.sh file.bat   (the .bat lives in cdrive/ ; runs headless in DOSBox-X with C: = cdrive)
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy HOME=/tmp/claude-0/dosbox/home
mkdir -p $HOME; cd /tmp/claude-0/dosbox
sed -i 's/\r$//; s/$/\r/' cdrive/$1
timeout ${T:-300} ./squashfs-root/AppRun -silent -nogui -nomenu -noconsole \
  -c "mount c /tmp/claude-0/dosbox/cdrive" -c "c:" -c "call $1" -c "exit" 2>&1 | grep -v "^LOG"
