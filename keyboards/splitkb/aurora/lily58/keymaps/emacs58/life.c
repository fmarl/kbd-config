// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>

#include QMK_KEYBOARD_H
#include "life.h"

// A row of 32 cells is one word, bit x is column x.
#define GRID_WIDTH  32
#define GRID_HEIGHT 128

#define MAX_GENERATIONS 1000

typedef uint32_t grid_t[GRID_HEIGHT];

static uint32_t rotate_left(uint32_t row) {
    return row << 1 | row >> (GRID_WIDTH - 1);
}

static uint32_t rotate_right(uint32_t row) {
    return row >> 1 | row << (GRID_WIDTH - 1);
}

// Counts the 8 neighbors of all cells in a row at once, one bit per cell:
// ones and twos hold the count in binary, fours marks counts of 4 and more.
static uint32_t next_row(uint32_t above, uint32_t row, uint32_t below) {
    uint32_t neighbors[] = {
        rotate_left(above), above, rotate_right(above),
        rotate_left(row),          rotate_right(row),
        rotate_left(below), below, rotate_right(below),
    };
    uint32_t ones = 0, twos = 0, fours = 0;

    for (uint8_t i = 0; i < ARRAY_SIZE(neighbors); i++) {
        uint32_t carry = ones & neighbors[i];
        ones ^= neighbors[i];
        fours |= twos & carry;
        twos ^= carry;
    }

    // Born with 3 neighbors, survives with 2 or 3.
    return twos & ~fours & (ones | row);
}

static void step(const grid_t current, grid_t next) {
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        uint32_t above = current[(y + GRID_HEIGHT - 1) % GRID_HEIGHT];
        uint32_t below = current[(y + 1) % GRID_HEIGHT];
        next[y]        = next_row(above, current[y], below);
    }
}

static uint32_t random_state;

// xorshift32
static uint32_t random_word(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

// A cell starts alive when two random bits are set, so about a quarter do.
static void seed(grid_t grid) {
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        grid[y] = random_word() & random_word();
    }
}

// The display takes pages of 8 rows, one byte per column with the top row
// in the lowest bit.
static void draw(const grid_t grid) {
    static uint8_t pages[GRID_WIDTH * GRID_HEIGHT / 8];

    for (uint8_t page = 0; page < GRID_HEIGHT / 8; page++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            uint8_t column = 0;
            for (uint8_t bit = 0; bit < 8; bit++) {
                column |= (grid[page * 8 + bit] >> x & 1) << bit;
            }
            pages[page * GRID_WIDTH + x] = column;
        }
    }
    oled_write_raw((const char *)pages, sizeof(pages));
}

static grid_t    grids[3];
static uint32_t *previous = grids[0];
static uint32_t *current  = grids[1];
static uint32_t *next     = grids[2];
static uint16_t  generation;

static void start_population(void) {
    seed(current);
    memset(previous, 0, sizeof(grid_t));
    generation = 0;
}

static void advance_generation(void) {
    step(current, next);
    // Still lifes and blinkers repeat after one or two generations.
    bool stuck = memcmp(next, current, sizeof(grid_t)) == 0 || memcmp(next, previous, sizeof(grid_t)) == 0;

    uint32_t *oldest = previous;
    previous         = current;
    current          = next;
    next             = oldest;

    if (stuck || ++generation == MAX_GENERATIONS) {
        start_population();
    }
}

void render_life(void) {
    static uint32_t generation_shown_at;
    static bool     restart = true;

    if (!is_oled_on()) {
        restart = true;
        return;
    }

    if (restart) {
        restart      = false;
        random_state = timer_read32() | 1;
        start_population();
    } else if (timer_elapsed32(generation_shown_at) >= LIFE_GENERATION_MS) {
        advance_generation();
    } else {
        return;
    }

    generation_shown_at = timer_read32();
    draw(current);
}
