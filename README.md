# Raumfahrt UART Chain

This folder contains the three firmware programs for the UART transmission chain:

- `blinky`: sender board, sends framed data with triple repetition and CRC-16
- `uart-noisemaker`: middle board that forwards bytes and can flip bits
- `hello-world`: receiver board, decodes triple repetition and checks CRC-16

All commands below assume the board is `nucleo-h753zi`.

## Find Serial Ports

List connected Nucleo serial ports:

```sh
ls -l /dev/ttyACM*
```

If you are unsure which board is which, unplug one board, run the command again,
then plug it back in. The new `/dev/ttyACM...` entry belongs to that board.

## Build Everything

From the RIOT root:

```sh
make -C examples/basic/raumfahrt/blinky BOARD=nucleo-h753zi
make -C examples/basic/raumfahrt/uart-noisemaker BOARD=nucleo-h753zi
make -C examples/basic/raumfahrt/hello-world BOARD=nucleo-h753zi
```

## Flash And Open Terminals

Use the correct `PORT=/dev/ttyACM...` for each physical board.

Sender board:

```sh
cd /home/kara/RIOT/examples/basic/raumfahrt/blinky
make BOARD=nucleo-h753zi PORT=/dev/ttyACM0 flash term
```

Noisemaker board:

```sh
cd /home/kara/RIOT/examples/basic/raumfahrt/uart-noisemaker
make BOARD=nucleo-h753zi PORT=/dev/ttyACM1 flash term
```

Receiver board:

```sh
cd /home/kara/RIOT/examples/basic/raumfahrt/hello-world
make BOARD=nucleo-h753zi PORT=/dev/ttyACM2 flash term
```

To open only the terminal without flashing again:

```sh
make BOARD=nucleo-h753zi PORT=/dev/ttyACM0 term
```

## Wiring

All UART data uses `UART_DEV(2)`:

- `PD5` = TX
- `PD6` = RX
- baud rate = `115200`

Three-board chain:

```text
Sender PD5 / TX       -> Noisemaker PD6 / RX
Noisemaker PD5 / TX   -> Receiver PD6 / RX

Sender GND            -> Noisemaker GND
Noisemaker GND        -> Receiver GND
```

For a direct two-board test without the noisemaker:

```text
Sender PD5 / TX       -> Receiver PD6 / RX
Sender GND            -> Receiver GND
```

## Noisemaker Shell Commands

These commands must be typed inside the RIOT shell of the noisemaker board,
not in the normal Bash terminal.

Print current configuration:

```text
print_cfg
```

Forward without errors:

```text
cfg chance 0
```

This should let the receiver print `OK ... Hello from board 1`.

Random bit flips:

```text
cfg type random
cfg chance 5
```

The receiver can correct many single-copy bit flips because each byte is sent
three times and decoded by majority vote. If too many copies of the same bit are
flipped, the CRC check should fail and the receiver prints a `CRC error`.

Fixed-distance bit flips:

```text
cfg type fixed
cfg dist 20
```

Burst errors:

```text
cfg type burst
cfg chance 5
cfg burstlen 4
```

Show all RIOT shell commands:

```text
help
```

## Clean Builds

If something worked before and now behaves strangely, clean and rebuild:

```sh
make -C /home/kara/RIOT/examples/basic/raumfahrt/blinky BOARD=nucleo-h753zi clean
make -C /home/kara/RIOT/examples/basic/raumfahrt/uart-noisemaker BOARD=nucleo-h753zi clean
make -C /home/kara/RIOT/examples/basic/raumfahrt/hello-world BOARD=nucleo-h753zi clean
```
