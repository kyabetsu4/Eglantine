#!/usr/bin/env bash
set -e
for f in /home/deck/pokopia-upstream/test/steam-controls.ini /home/deck/pokopia-upstream/test/deck-controls.ini; do
    sed -i 's/frame_gen=false/frame_gen=true/' "$f"
    sed -i 's/frame_gen\\default=true/frame_gen\\default=false/' "$f"
    sed -i 's/frame_gen_type=1/frame_gen_type=0/' "$f"
    sed -i 's/frame_gen_type\\default=true/frame_gen_type\\default=false/' "$f"
done
echo LSFG_ENABLED_OK
