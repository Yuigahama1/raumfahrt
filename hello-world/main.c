#include <stdint.h>
#include <stdio.h>

#include "periph/uart.h"
#include "ringbuffer.h"

#define LINK_UART       UART_DEV(2)
#define LINK_BAUDRATE   115200

#define FRAME_STX       0x7e
#define MAX_PAYLOAD     64
#define REPEAT_COUNT    5

typedef enum {
    WAIT_STX,
    READ_LEN,
    READ_BODY,
} parser_state_t;

static char rx_mem[256];
static ringbuffer_t rx_buf;

static parser_state_t state = WAIT_STX;
static uint8_t rep_buf[REPEAT_COUNT];
static uint8_t rep_pos;
static uint8_t payload[MAX_PAYLOAD];
static uint8_t payload_len;
static uint8_t body[(MAX_PAYLOAD + 2) * REPEAT_COUNT];
static size_t body_pos;
static size_t body_expected;

static uint32_t frames_ok;
static uint32_t frames_crc_err;
static uint32_t frames_framing_err;
static uint32_t corrected_bits;

static void link_rx_cb(void *arg, uint8_t data)
{
    (void)arg;
    ringbuffer_add_one(&rx_buf, data);
}

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

static uint8_t decode_repetition(const uint8_t *in)
{
    uint8_t out = 0;

    for (unsigned bit = 0; bit < 8; bit++) {
        unsigned ones = 0;

        for (unsigned i = 0; i < REPEAT_COUNT; i++) {
            ones += (in[i] >> bit) & 1;
        }

        if ((ones != 0) && (ones != REPEAT_COUNT)) {
            corrected_bits++;
        }

        if (ones > (REPEAT_COUNT / 2)) {
            out |= (uint8_t)(1U << bit);
        }
    }

    return out;
}

static void reset_parser(void)
{
    state = WAIT_STX;
    rep_pos = 0;
    payload_len = 0;
    body_pos = 0;
    body_expected = 0;
}

static void decode_and_print_frame(void)
{
    uint8_t decoded[MAX_PAYLOAD + 2];
    size_t decoded_len = payload_len + 2;

    for (size_t i = 0; i < decoded_len; i++) {
        decoded[i] = decode_repetition(&body[i * REPEAT_COUNT]);
    }

    for (uint8_t i = 0; i < payload_len; i++) {
        payload[i] = decoded[i];
    }

    uint16_t crc_rx = (uint16_t)decoded[payload_len] |
                      ((uint16_t)decoded[payload_len + 1] << 8);
    uint16_t crc_calc = crc16_ccitt_false_calc(payload, payload_len);

    if (crc_rx == crc_calc) {
        frames_ok++;
        printf("OK %lu: ", (unsigned long)frames_ok);
        for (uint8_t i = 0; i < payload_len; i++) {
            putchar((char)payload[i]);
        }
        printf("stats ok=%lu crc_err=%lu framing_err=%lu corrected_bits=%lu\n",
               (unsigned long)frames_ok,
               (unsigned long)frames_crc_err,
               (unsigned long)frames_framing_err,
               (unsigned long)corrected_bits);
    }
    else {
        frames_crc_err++;
        printf("CRC error: rx=0x%04x calc=0x%04x stats ok=%lu crc_err=%lu framing_err=%lu corrected_bits=%lu\n",
               (unsigned)crc_rx,
               (unsigned)crc_calc,
               (unsigned long)frames_ok,
               (unsigned long)frames_crc_err,
               (unsigned long)frames_framing_err,
               (unsigned long)corrected_bits);
    }
}

static void parse_byte(uint8_t byte)
{
    switch (state) {
    case WAIT_STX:
        if (byte == FRAME_STX) {
            rep_pos = 0;
            state = READ_LEN;
        }
        break;

    case READ_LEN:
        rep_buf[rep_pos++] = byte;
        if (rep_pos == sizeof(rep_buf)) {
            payload_len = decode_repetition(rep_buf);
            if ((payload_len == 0) || (payload_len > MAX_PAYLOAD)) {
                frames_framing_err++;
                reset_parser();
            }
            else {
                body_pos = 0;
                body_expected = (payload_len + 2) * REPEAT_COUNT;
                state = READ_BODY;
            }
        }
        break;

    case READ_BODY:
        body[body_pos++] = byte;
        if (body_pos == body_expected) {
            decode_and_print_frame();
            reset_parser();
        }
        break;
    }
}

int main(void)
{
    puts("Board 2 receiver started");
    puts("Listening for encoded frames on UART_DEV(2): TX=PD5, RX=PD6");

    ringbuffer_init(&rx_buf, rx_mem, sizeof(rx_mem));

    if (uart_init(LINK_UART, LINK_BAUDRATE, link_rx_cb, NULL) != UART_OK) {
        puts("Failed to initialize link UART");
        return 1;
    }

    while (1) {
        int c = ringbuffer_get_one(&rx_buf);

        if (c >= 0) {
            parse_byte((uint8_t)c);
        }
    }
}
