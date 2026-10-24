#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quantum.h"
#include "life.h"

#define DISPLAY_WIDTH  32
#define DISPLAY_HEIGHT 128
#define PAGE_HEIGHT    8
#define FONT_WIDTH     6
#define CHARS_PER_LINE (DISPLAY_WIDTH / FONT_WIDTH)
#define LINE_COUNT     (DISPLAY_HEIGHT / PAGE_HEIGHT)
#define LIFE_FRAMES    200

extern const unsigned char font[];

layer_state_t layer_state;

static uint8_t  oled_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / PAGE_HEIGHT];
static uint8_t  cursor_column, cursor_line;
static uint8_t  active_mods;
static bool     caps_word_on;
static bool     keyboard_master;
static uint32_t clock_ms;

uint8_t get_highest_layer(layer_state_t state) {
    uint8_t highest = 0;
    
    for (uint8_t layer = 0; layer < 32; layer++) {
        if (state & (1UL << layer)) {
            highest = layer;
        }
    }
    
    return highest;
}

uint8_t  get_mods(void) { return active_mods; }
bool     is_caps_word_on(void) { return caps_word_on; }
bool     is_keyboard_master(void) { return keyboard_master; }
uint32_t timer_read32(void) { return clock_ms; }
uint32_t timer_elapsed32(uint32_t start) { return clock_ms - start; }
bool     is_oled_on(void) { return true; }

void oled_set_cursor(uint8_t column, uint8_t line) {
    cursor_column = column;
    cursor_line   = line;
}

void oled_write_char(const char data, bool invert) {
    unsigned char character = data;
    if (character < OLED_FONT_START || character > OLED_FONT_END) {
        character = ' ';
    }
    
    const unsigned char *glyph = &font[(character - OLED_FONT_START) * FONT_WIDTH];
    uint8_t *target = &oled_buffer[cursor_line * DISPLAY_WIDTH + cursor_column * FONT_WIDTH];
    for (int x = 0; x < FONT_WIDTH; x++) {
        target[x] = invert ? ~glyph[x] : glyph[x];
    }
    
    if (++cursor_column >= CHARS_PER_LINE) {
        cursor_column = 0;
        cursor_line   = (cursor_line + 1) % LINE_COUNT;
    }
}

void oled_write(const char *text, bool invert) {
    while (*text) {
        oled_write_char(*text++, invert);
    }
}

void oled_write_P(const char *text, bool invert) {
    oled_write(text, invert);
}

void oled_write_raw_byte(const char data, uint16_t index) {
    oled_buffer[MIN(index, sizeof(oled_buffer) - 1)] = data;
}

void oled_write_raw(const char *data, uint16_t size) {
    memcpy(oled_buffer, data, MIN(size, sizeof(oled_buffer)));
}

static bool is_lit(int x, int y) {
    return oled_buffer[(y / PAGE_HEIGHT) * DISPLAY_WIDTH + x] & (1 << (y % PAGE_HEIGHT));
}

static void write_pbm(const char *dir, const char *name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.pbm", dir, name);
    FILE *file = fopen(path, "w");

    if (!file) {
        perror(path);
        exit(1);
    }
    
    fprintf(file, "P1\n%d %d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            fputs(is_lit(x, y) ? "0 " : "1 ", file); // PBM: 1 is black
        }
        fputc('\n', file);
    }
    
    fclose(file);
}

typedef struct {
    uint8_t  mods;
    uint16_t keycode;
} key_press_t;

// presses ends with a KC_NO entry.
static void capture_status(const char *dir, const char *name, uint8_t layer,
                           bool caps_word, const key_press_t presses[]) {
    keyrecord_t record = {.event = {.pressed = true}};

    clock_ms += 60000;
    memset(oled_buffer, 0, sizeof(oled_buffer));
    layer_state     = layer ? 1UL << layer : 0;
    caps_word_on    = caps_word;
    keyboard_master = true;
    
    for (const key_press_t *press = presses; press->keycode != KC_NO; press++) {
        active_mods = press->mods;
        pre_process_record_user(press->keycode, &record);
    }
    
    active_mods = 0;
    oled_task_user();
    write_pbm(dir, name);
}

static void capture_animation(const char *dir) {
    keyboard_master = false;
    
    for (int frame = 0; frame < LIFE_FRAMES; frame++) {
        char name[32];
        oled_task_user();
        snprintf(name, sizeof(name), "frame-%04d", frame);
        write_pbm(dir, name);
        clock_ms += LIFE_GENERATION_MS;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIR\n", argv[0]);
        return 1;
    }
    
    const char *output_dir = argv[1];

    capture_status(output_dir, "status-1-BASE, C-x b", 0, false,
                   (key_press_t[]){{MOD_BIT_LCTRL, KC_X}, {0, KC_B}, {0, KC_NO}});
    capture_status(output_dir, "status-2-EMACS, C-x 2", 1, false,
                   (key_press_t[]){{0, QK_MACRO_2}, {0, KC_NO}});
    capture_status(output_dir, "status-3-FN, C-M-SPC", 2, false,
                   (key_press_t[]){{MOD_BIT_LCTRL | MOD_BIT_LALT, KC_SPACE}, {0, KC_NO}});
    capture_status(output_dir, "status-4-Caps Word, M-%", 0, true,
                   (key_press_t[]){{MOD_BIT_LALT, KC_PERC}, {0, KC_NO}});
    capture_status(output_dir, "status-5-BASE, C-x C-s", 0, false,
                   (key_press_t[]){{MOD_BIT_LCTRL, KC_X}, {MOD_BIT_LCTRL, KC_S}, {0, KC_NO}});
    capture_animation(output_dir);

    printf("%d\n", LIFE_GENERATION_MS);
    
    return 0;
}
