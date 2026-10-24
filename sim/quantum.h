#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "modifiers.h"
#include "progmem.h"
#include "quantum_keycodes.h"

#define MIN(a, b)     ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define OLED_FONT_START  0
#define OLED_FONT_END    223
#define OLED_MATRIX_SIZE (32 * 128 / 8)

typedef uint32_t layer_state_t;

typedef struct {
    struct {
        bool pressed;
    } event;
} keyrecord_t;

extern layer_state_t layer_state;

uint8_t  get_highest_layer(layer_state_t state);
uint8_t  get_mods(void);
bool     is_caps_word_on(void);
bool     is_keyboard_master(void);
uint32_t timer_read32(void);
uint32_t timer_elapsed32(uint32_t start);

bool is_oled_on(void);
void oled_set_cursor(uint8_t column, uint8_t line);
void oled_write_char(const char data, bool invert);
void oled_write(const char *text, bool invert);
void oled_write_P(const char *text, bool invert);
void oled_write_raw_byte(const char data, uint16_t index);
void oled_write_raw(const char *data, uint16_t size);

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record);
bool oled_task_user(void);
