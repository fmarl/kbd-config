#!/bin/sh

set -eu

if [ $# -ne 1 ]; then
    echo "usage: $0 OUTPUT_DIR" >&2
    exit 1
fi

output_dir=$1
emacs_dir=$(dirname "$(readlink -f "$(command -v emacs)")")/..
emacs_icon=${EMACS_ICON:-$emacs_dir/share/icons/hicolor/128x128/apps/emacs.png}
sans_bold=$(fc-match -f '%{file}' 'DejaVu Sans:bold')
serif_bold_italic=$(fc-match -f '%{file}' 'DejaVu Serif:bold:italic')
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

fit() {
    convert "$1" -trim +repage -resize 28x28 -colorspace Gray -threshold 50% \
            -compose over -background black -gravity center -extent 32x32 \
            -type bilevel "$2"
}

glyph() {
    font=$1 size=$2 text=$3 name=$4
    convert -size 256x256 xc:black -fill white -font "$font" -pointsize "$size" \
            -gravity center -annotate +0+0 "$text" "$work_dir/$name.png"
    fit "$work_dir/$name.png" "$output_dir/$name.png"
}

mkdir -p "$output_dir"
glyph "$sans_bold" 220 λ lambda
glyph "$sans_bold" 220 Λ capital_lambda
glyph "$serif_bold_italic" 200 ƒ function

convert "$emacs_icon" -alpha extract -threshold 50% "$work_dir/disc.png"
convert "$emacs_icon" -background black -alpha remove -colorspace Gray -threshold 60% \
        -negate "$work_dir/disc.png" -compose multiply -composite \
        -negate "$work_dir/disc.png" -compose multiply -composite "$work_dir/emacs.png"

fit "$work_dir/emacs.png" "$output_dir/emacs.png"
