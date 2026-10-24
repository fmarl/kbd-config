#!/bin/sh
# Usage: oled-preview.sh QMK_HOME KEYMAP_DIR OUTPUT_DIR

set -eu

if [ $# -ne 3 ]; then
    echo "usage: $0 QMK_HOME KEYMAP_DIR OUTPUT_DIR" >&2
    exit 1
fi

qmk_home=$1
splitkb_dir="$qmk_home/keyboards/splitkb/aurora/lily58"
keymap_dir=$2
output_dir=$3
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

gcc -std=gnu11 -O1 -Wall -DQMK_KEYBOARD_H='"quantum.h"' \
    -Isim -I"$keymap_dir" \
    -I"$qmk_home/quantum" -I"$qmk_home/quantum/keymap_extras" -I"$qmk_home/quantum/sequencer" \
    -o "$work_dir/sim" \
    sim/sim.c "$keymap_dir/oled.c" "$keymap_dir/life.c" "$splitkb_dir/glcdfont.c"
frame_ms=$("$work_dir/sim" "$work_dir")

mkdir -p "$output_dir"

for screen in "$work_dir"/status-*.pbm "$work_dir"/frame-*.pbm; do
    screen_name=$(basename "$screen" .pbm)
    convert "$screen" -filter point -resize 400% \
            -bordercolor '#222' -border 8 -bordercolor '#555' -border 2 \
            -set label "${screen_name#status-?-}" "${screen%.pbm}.png"
done

montage "$work_dir"/status-*.png -tile x1 -geometry +12+12 -background white \
        "$output_dir/status.png"

convert -delay $((frame_ms / 10)) -loop 0 "$work_dir"/frame-*.png \
        "$output_dir/animation.gif"

echo "$output_dir/status.png"
echo "$output_dir/animation.gif"
