SRC += oled.c life.c

# Reports a press right away and ignores the bouncing after it.
DEBOUNCE_TYPE = sym_eager_pk

# Unused: no RGB LEDs or encoders fitted, and no mouse, Grave Escape, Space
# Cadet or Magic keycodes in the keymap.
RGB_MATRIX_ENABLE  = no
ENCODER_ENABLE     = no
MOUSEKEY_ENABLE    = no
GRAVE_ESC_ENABLE   = no
SPACE_CADET_ENABLE = no
MAGIC_ENABLE       = no
