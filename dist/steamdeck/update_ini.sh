#!/usr/bin/env bash
set -e
for f in /home/deck/pokopia-upstream/test/steam-controls.ini /home/deck/pokopia-upstream/test/deck-controls.ini; do
    sed -i 's/async_presentation=true/async_presentation=false/' "$f"
    sed -i 's/async_presentation\\default=false/async_presentation\\default=true/' "$f"
    sed -i 's/anti_aliasing=1/anti_aliasing=0/' "$f"
    sed -i 's/anti_aliasing\\default=true/anti_aliasing\\default=false/' "$f"
done
chmod +x /home/deck/pokopia-upstream/launch-pokopia.py
echo INI_RESTORE_DONE
