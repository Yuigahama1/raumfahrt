#include <stdio.h>
#include <string.h>

#include "periph/uart.h"
#include "timex.h"
#include "ztimer.h"

#define LINK_UART       UART_DEV(2)
#define LINK_BAUDRATE   115200

static const char message[] = "Hello from board 1\r\n";

int main(void)
{
    puts("Board 1 sender started");
    puts("Sending on UART_DEV(2): TX=PD5, RX=PD6");

    if (uart_init(LINK_UART, LINK_BAUDRATE, NULL, NULL) != UART_OK) {
        puts("Failed to initialize link UART");
        return 1;
    }

    while (1) {
        uart_write(LINK_UART, (const uint8_t *)message, strlen(message));
        puts("sent");
        ztimer_sleep(ZTIMER_USEC, US_PER_SEC);
    }

    return 0;
}
