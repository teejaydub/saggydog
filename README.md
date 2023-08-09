# saggydog
Arduino firmware to monitor voltages and log their values to a serial port.

## Building

Should be widely compatible with hardware supporting Arduino.

Set up for use with PlatformIO; includes a VSCode workspace.

A build target for the ESP32-C3 DevKit M-1 board is included.  This mainly
required some customizations to the ESP-IDF `sdkconfig`, to prevent logging
other stuff to the serial port.

## Configuration

The key settings are currently in the source code, in `main.cpp`, in the `Feature configuration` section.

It would be cool to replace those with a console interface via serial input,
that would save the last-used settings to non-volatile storage.

Defaults are currently to log the voltage on the Arduino pin `A1` every 4 seconds,
and to go to "warning" mode below 3.0 V and to "error" mode below 2.7 V.

## Usage

Plug the programmed board into USB, or otherwise power it and connect it to a serial port.

Monitor the serial output via a console program.

## Tricks

You can use my other project [Retrover](https://github.com/teejaydub/retrover)
to monitor the serial output interleaved with the log from another board under development.

I found this particularly useful for diagnosing problems caused by brownouts on a power supply.
Retrover's output merges Saggydog's with the output of the board under test,
so when a brownout caused the chip to reboot, the Retrover log looked like this:

```
2023-08-07 16:42:53.409611  > ~S=4 ?=1 ?*
2023-08-07 16:42:53.741868 <  3.30 V
2023-08-07 16:42:54.414825  > ~S=4 ?=1 ?*
2023-08-07 16:42:54.798462 <  2.98 V ??
2023-08-07 16:42:55.890083 <  2.79 V ??
2023-08-07 16:42:57.106462  > CB 7.9.2
2023-08-07 16:42:57.106462  > ~S=2 ?=1 ?P ?*
2023-08-07 16:42:57.108566

== EVENT FOUND ==

2023-08-07 16:42:57.383441  > [P.]
2023-08-07 16:42:58.101074  > ~S=2 ?=1 ?*
2023-08-07 16:42:59.191474  > ~S=2 ?=1 ?*
2023-08-07 16:42:59.797474 <  3.30 V
```

Retrover labels the Saggydog output with `< ` and the test board's output with ` >`.

Normal output at nominal supply voltage (3.3 V) is then followed by dropping
voltage, flagged with `??` and accompanied by a faster reporting rate,
followed in turn by the message `CB 7.9.2` which is the version number of the
board under test, output when the board reboots.  After rebooting, supply
voltage returns to 3.30 V.