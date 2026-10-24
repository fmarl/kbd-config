// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>

#include QMK_KEYBOARD_H
#include "capital_lambda_symbol.h"
#include "emacs_symbol.h"
#include "function_symbol.h"
#include "lambda_symbol.h"
#include "life.h"

enum layers { BASE, EMACS, FN };

#define BLANK_LINE "     "
#define LINE_CHARS (sizeof(BLANK_LINE) - 1)
#define ECHO_LINE  8
#define ECHO_LINES 3
#define ECHO_MS    3000
#define CHORD_SIZE sizeof("C-M-S-SPC")
#define ECHO_SIZE  sizeof("C-x C-M-S-SPC")

#define SYMBOL(name) .symbol = name, .symbol_size = sizeof(name)

static const struct {
    const char  icon[16];
    const char *symbol;
    uint16_t    symbol_size;
} layer_displays[] = {
    [BASE] = {{0x20, 0x9a, 0x9b, 0x9c, 0x20,
               0x20, 0xba, 0xbb, 0xbc, 0x20,
               0x20, 0xda, 0xdb, 0xdc, 0x20},
              SYMBOL(lambda_symbol)},
    [EMACS] = {{0x20, 0x94, 0x95, 0x96, 0x20,
                0x20, 0xb4, 0xb5, 0xb6, 0x20,
                0x20, 0xd4, 0xd5, 0xd6, 0x20},
               SYMBOL(emacs_symbol)},
    [FN] = {{0x20, 0x97, 0x98, 0x99, 0x20,
             0x20, 0xb7, 0xb8, 0xb9, 0x20,
             0x20, 0xd7, 0xd8, 0xd9, 0x20},
            SYMBOL(function_symbol)},
};

static const char macro_keys[] = "0123ok";

static const char *const prefixes[]   = {"C-x", "C-c", "C-h", "M-g", "M-s"};
static const char *const named_keys[] = {"RET", "ESC", "DEL", "TAB", "SPC"};

static const char unshifted_chars[] = "abcdefghijklmnopqrstuvwxyz1234567890     -=[]\\ ;'`,./";
static const char shifted_chars[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()     _+{}| :\"~<>?";

static char     echo_text[ECHO_SIZE];
static uint32_t echo_shown_at;

static bool echo_visible(void) {
    return echo_text[0] && timer_elapsed32(echo_shown_at) < ECHO_MS;
}

static bool prefix_pending(void) {
    if (!echo_visible()) {
        return false;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(prefixes); i++) {
        if (strcmp(echo_text, prefixes[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

static void push_chord(const char *chord) {
    if (prefix_pending()) {
        strcat(echo_text, " ");
    } else {
        echo_text[0] = '\0';
    }
    
    strcat(echo_text, chord);
    echo_shown_at = timer_read32();
}

// Keycodes like LCA(KC_B) carry 5-bit mods, with the right-hand flag in bit 4.
static uint8_t keycode_mods(uint16_t keycode) {
    uint8_t mods = QK_MODS_GET_MODS(keycode);
    
    return mods & 0x10 ? (mods & 0x0F) << 4 : mods;
}

static bool format_chord(uint16_t keycode, char chord[CHORD_SIZE]) {
    uint8_t mods = get_mods();
    if (IS_QK_MODS(keycode)) {
        mods |= keycode_mods(keycode);
        keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
    }
    
    if (keycode < KC_A || keycode > KC_SLASH || mods & MOD_MASK_GUI) {
        return false;
    }

    bool ctrl = mods & MOD_MASK_CTRL;
    bool meta = mods & MOD_BIT_LALT;
    if (!ctrl && !meta && !prefix_pending()) {
        return false;
    }

    bool named        = keycode >= KC_ENTER && keycode <= KC_SPACE;
    bool shift        = mods & MOD_MASK_SHIFT;
    bool shift_prefix = shift && (named || (ctrl && keycode <= KC_Z));

    char key[2] = {(shift && !shift_prefix ? shifted_chars : unshifted_chars)[keycode - KC_A], '\0'};
    if (!named && key[0] == ' ') {
        return false;
    }

    strcpy(chord, ctrl ? "C-" : "");
    strcat(chord, meta ? "M-" : "");
    strcat(chord, shift_prefix ? "S-" : "");
    strcat(chord, named ? named_keys[keycode - KC_ENTER] : key);
    
    return true;
}

static void echo_key(uint16_t keycode) {
    char chord[CHORD_SIZE];

    if (keycode >= QK_MACRO_0 && keycode < QK_MACRO_0 + strlen(macro_keys)) {
        echo_text[0] = '\0';
        push_chord("C-x");
        push_chord((char[]){macro_keys[keycode - QK_MACRO_0], '\0'});
    } else if (format_chord(keycode, chord)) {
        push_chord(chord);
    }
}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        echo_key(keycode);
    }
    
    return true;
}

static size_t line_length(const char *text) {
    size_t length = strlen(text);
    if (length <= LINE_CHARS) {
        return length;
    }
    
    for (size_t i = LINE_CHARS; i > 0; i--) {
        if (text[i] == ' ' || text[i - 1] == '-') {
            return i;
        }
    }
    
    return LINE_CHARS;
}

static void render_echo(void) {
    const char *text = echo_visible() ? echo_text : "";

    for (uint8_t line = 0; line < ECHO_LINES; line++) {
        size_t length = line_length(text);

        oled_set_cursor(0, ECHO_LINE + line);
        for (size_t column = 0; column < LINE_CHARS; column++) {
            oled_write_char(column < length ? text[column] : ' ', false);
        }
        
        text += length;
        if (*text == ' ') {
            text++;
        }
    }
}

static void render_symbol(const char *symbol, uint16_t size) {
    uint16_t offset = OLED_MATRIX_SIZE - size;

    for (uint16_t i = 0; i < size; i++) {
        oled_write_raw_byte(pgm_read_byte(&symbol[i]), offset + i);
    }
}

static void render_status(void) {
    uint8_t layer = MIN(get_highest_layer(layer_state), FN);

    oled_set_cursor(0, 0);
    oled_write_P(PSTR(BLANK_LINE), false);
    oled_write(layer_displays[layer].icon, false);
    render_echo();
    
    if (is_caps_word_on()) {
        render_symbol(capital_lambda_symbol, sizeof(capital_lambda_symbol));
    } else {
        render_symbol(layer_displays[layer].symbol, layer_displays[layer].symbol_size);
    }
}

bool oled_task_user(void) {
    if (is_keyboard_master()) {
        render_status();
    } else {
        render_life();
    }

    return false;
}
