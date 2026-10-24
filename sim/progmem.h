#pragma once

#define PROGMEM
#define PSTR(s)          (s)
#define pgm_read_byte(p) (*(const uint8_t *)(p))
