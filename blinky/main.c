#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "periph/uart.h"
#include "timex.h"
#include "ztimer.h"

#define LINK_UART       UART_DEV(2)
#define LINK_BAUDRATE   115200

#define FRAME_STX       0x7e
#define MAX_PAYLOAD     64
#define REPEAT_COUNT    5
#define BYTE_DELAY_US   1000

static const char message[] = "Hello from board 1\r\n";

static uint16_t crc16_ccitt_false_calc(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xffff;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;

        for (unsigned bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            }
            else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static void add_repetition(uint8_t *out, size_t *pos, uint8_t byte)
{
    for (unsigned i = 0; i < REPEAT_COUNT; i++) {
        out[(*pos)++] = byte;
    }
}

static size_t build_frame(uint8_t *out, const uint8_t *payload, uint8_t len)
{
    size_t pos = 0;
    uint16_t crc = crc16_ccitt_false_calc(payload, len);

    out[pos++] = FRAME_STX;
    add_repetition(out, &pos, len);

    for (uint8_t i = 0; i < len; i++) {
        add_repetition(out, &pos, payload[i]);
    }

    add_repetition(out, &pos, (uint8_t)(crc & 0xff));
    add_repetition(out, &pos, (uint8_t)(crc >> 8));

    return pos;
}

static void write_frame_slowly(uart_t uart, const uint8_t *frame, size_t frame_len)
{
    for (size_t i = 0; i < frame_len; i++) {
        uart_write(uart, &frame[i], 1);
        ztimer_sleep(ZTIMER_USEC, BYTE_DELAY_US);
    }
}

int main(void)
{
    uint8_t frame[1 + (REPEAT_COUNT * (1 + MAX_PAYLOAD + 2))];
    const uint8_t payload_len = (uint8_t)strlen(message);

    puts("Board 1 sender started");
    puts("Sending encoded frames on UART_DEV(2): TX=PD5, RX=PD6");

    if (payload_len > MAX_PAYLOAD) {
        puts("Message is too long");
        return 1;
    }

    if (uart_init(LINK_UART, LINK_BAUDRATE, NULL, NULL) != UART_OK) {
        puts("Failed to initialize link UART");
        return 1;
    }

    while (1) {
        size_t frame_len = build_frame(frame, (const uint8_t *)message, payload_len);
        write_frame_slowly(LINK_UART, frame, frame_len);
        printf("sent encoded frame (%u payload bytes, %u wire bytes)\n",
               (unsigned)payload_len, (unsigned)frame_len);
        ztimer_sleep(ZTIMER_USEC, US_PER_SEC);
    }

    return 0;
}
