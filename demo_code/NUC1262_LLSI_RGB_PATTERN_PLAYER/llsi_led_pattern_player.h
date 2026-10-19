#ifndef LLSI_LED_PATTERN_PLAYER_H
#define LLSI_LED_PATTERN_PLAYER_H

#include <stdint.h>
#include "llsi_led_pattern_drv.h"

#define G_RGB_PATTERN_LED_COUNT (4u)
#define G_RGB_PATTERN_FRAME_COUNT (2u)

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} llsi_led_pattern_color_t;

typedef struct
{
    uint16_t duration_ms;
    llsi_led_pattern_color_t led[G_RGB_PATTERN_LED_COUNT];
} llsi_led_pattern_frame_t;

void g_rgb_pattern_render_frame(const llsi_led_pattern_frame_t *frame);
void g_rgb_pattern_play_once(void);
uint16_t g_rgb_pattern_step(uint16_t frame_index);

#endif
