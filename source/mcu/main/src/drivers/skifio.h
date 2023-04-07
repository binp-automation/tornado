/*!
 * @brief Driver for Tornado DAC/ADC board.
 */

#pragma once

#include <hal/defs.h>

#define SKIFIO_ADC_CHANNEL_COUNT 6

#define SKIFIO_DIN_SIZE 8
#define SKIFIO_DOUT_SIZE 4

typedef int32_t SkifioAin;
typedef int16_t SkifioAout;

typedef struct SkifioInput {
    SkifioAin adcs[SKIFIO_ADC_CHANNEL_COUNT];
    int8_t temp;
    uint8_t status;
} SkifioInput;

typedef struct SkifioOutput {
    SkifioAout dac;
} SkifioOutput;

typedef uint8_t SkifioDin;
typedef uint8_t SkifioDout;
typedef void (*SkifioDinCallback)(void *, SkifioDin);

hal_retcode skifio_init();
hal_retcode skifio_deinit();

hal_retcode skifio_dac_enable();
hal_retcode skifio_dac_disable();

hal_retcode skifio_transfer(const SkifioOutput *out, SkifioInput *in);
hal_retcode skifio_wait_ready(uint32_t delay_ms);

hal_retcode skifio_dout_write(SkifioDout value);

SkifioDin skifio_din_read();
hal_retcode skifio_din_subscribe(SkifioDinCallback callback, void *data);
hal_retcode skifio_din_unsubscribe();
