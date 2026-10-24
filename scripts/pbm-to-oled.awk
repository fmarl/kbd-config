# Prints a plain PBM image as C array elements in the OLED's memory layout:
# pages of 8 pixel rows from top to bottom, one byte per column and page,
# with the topmost pixel in the least significant bit.

{
    sub(/#.*/, "")
    text = text " " $0
}

END {
    token_count = split(text, tokens)
    if (tokens[1] != "P1") {
        print "pbm-to-oled: expected a plain PBM image" > "/dev/stderr"
        exit 1
    }
    width  = tokens[2]
    height = tokens[3]

    pixels = ""
    for (i = 4; i <= token_count; i++) {
        pixels = pixels tokens[i]
    }

    for (page = 0; page < height / 8; page++) {
        line = "   "
        for (x = 0; x < width; x++) {
            line = line sprintf(" 0x%02x,", column_byte(x, page))
        }
        print line
    }
}

function column_byte(x, page,    bit, byte) {
    byte = 0
    for (bit = 0; bit < 8; bit++) {
        if (is_lit(x, page * 8 + bit)) {
            byte += 2 ^ bit
        }
    }
    return byte
}

# PBM uses 1 for black.
function is_lit(x, y) {
    return substr(pixels, y * width + x + 1, 1) == "0"
}
