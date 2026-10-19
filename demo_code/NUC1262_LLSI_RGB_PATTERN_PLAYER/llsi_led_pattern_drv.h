#ifndef LLSI_LED_PATTERN_DRV_H
#define LLSI_LED_PATTERN_DRV_H

#include <stdint.h>

#ifndef LED_NUM
#define LED_NUM (4u)
#endif

#ifndef LED_DATA_LEN
#define LED_DATA_LEN (((LED_NUM * 3u) + 3u) / 4u)
#endif

#define LLSI_LED_PATTERN_COLOR_ORDER_RGB (0u)
#define LLSI_LED_PATTERN_COLOR_ORDER_GRB (1u)

#ifndef LLSI_LED_PATTERN_COLOR_ORDER
#define LLSI_LED_PATTERN_COLOR_ORDER LLSI_LED_PATTERN_COLOR_ORDER_RGB
#endif

void LLSI_LED_PATTERN_DRV_Clear(void);
void LLSI_LED_PATTERN_DRV_SetPixel(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void LLSI_LED_PATTERN_DRV_Show(void);
void LLSI_LED_PATTERN_DRV_DelayMs(uint16_t ms);

#endif
