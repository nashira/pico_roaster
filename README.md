## Pico Roaster Firmware

The goal of this project is to replace the thermometer that came with my coffee roaster with custom electronics that will provide an interface to roast logging software, such as Artisan Scope.  Additionally, I've added an OLED display, Wifi connectivity and sensors for fan voltage and gas pressure.

<img src="https://github.com/nashira/pico_roaster/assets/757096/2b4ea559-d551-4cd7-9407-17ff3c79067c" width="240" height="320">

### Hardware

**Parts**

  * Raspberry Pi Pico
  * ESP01 wifi
  * (2) Adafruit MAX31855 thermocouple amplifiers
  * AS5600 magnetic rotary encoder
  * 64x128 oled display
  * 7805 linear regulator
  * terminal block breakout
  * misc. wires, resistors, etc.

The thermocouple amps and OLED display are pretty straight-forward.  

For wifi, I would have used a Pico W If I had been able to get one at the time.  Instead I used an ESP01 I had around, which does have the benefit of being separate from the MCU so I can position the attena outside the metal housing for better signal.  

The linear regulator is used to step-down the 24v DC that was already wired into the roaster for the drum motor to 5v to power the electronics.  This is pretty much at it's limit and it does get warm, but with a heatsink it's worked fine for over a year so far.

The AS5600 is used to capture the gas pressure reading off of an analog dial manometer.  The 4mm magnet that came with the sensor fits perfectly on the dial needle and is attached with a small drop of CA glue.  The needle and all surrounding materials are non-magnetic and the magnet doesn't appear to interfere with the guage's operation.  I 3D printed a new cap for the gauge to hold the sensor at the proper distance.  One note here: the magnetic field will interact with the pins on the sensor board if it is too close.  Because the needle has such low spring forces, the magnet/needle gets stuck in certain positions, because of this I had to increase the distance from the magnet to the sensor.  I may desolder the pins and solder the wires directly to the board as a better fix for the issue.

For measuring fan voltage, I just use a voltage divider with 2 resistors, ~14k and ~2k ohm, for an output of about 3.2v at 24v input.

### Software

The software is Arduino-based for all the excellent libraries available, but uses some Pico SDKs directly to make better use of the hardware.

The code is split up into components based on functionality (network, display, temperature, sensors, os).  Each component has a header for the public API and a source file for the implementation.  Each component is structured as a set of `tasks` that are scheduled and executed by the `os`.  The "operating system" is the pico-specific code and is really just a set of event queues (one for each core) and some functions for scheduling, cancelling and executing tasks.  I considered using FreeRTOS, but there were a couple reasons why I decided to build an event queue instead: 
  
  1) The tick rate of FreeRTOS is designed to be about 1khz.  I want to sample several sources at 1khz or more.  I've seen that this can be done sort of on the side, and FreeRTOS can be used for other, lower frequency tasks, but that would mean I still need to build something for the higher-frequency tasks.
  2) I don't think I really need or want preemptive multitasking.  I understand FreeRTOS can be used in a cooperative style, but I think it would be additional complexity without much benefit.  Having never used it before, I think the time-cost is too unpredictable and possibly too high.

**"OS"**

Each task is contained to a single core; this simplifies tasks' concurrency handling by making them effectively single-threaded.  It also helps avoid any problems Arduino libraries may have being executed from mulitple cores.  

I have designated `core0` for the slower tasks of updating the display, handling network requests* and any USB serial output, which may take 10s of ms.  `core1` executes the faster tasks of reading from the temperature probes, reading the fan voltage and reading the gas pressure, which typically take only micro-seconds to run, but run at a much higher frequency. 

*There is so much variability to task execution time for network handling and I didn't want to introduce too much latency so I couldn't find a good task frequency for network request handling.  Network requests are actually polled in serial with the task queue, although network initialization is still done through tasks scheduled by the os.

The `.ino` entrypoint uses `setup` and `setup1` to initialize core0 and core1, respectively and calls the `init` function exposed by each component.

**Network**

I use a library called 'picohttpparser' for parsing HTTP requests, plus some extra code I wrote for query string parsing.  The firmware has both an HTTP api, and a WebSockets api that is compatible with Artisan Scope.  It handles multiple, concurrent clients, limited by the ESP01 to 5.  I've benchmarked it at around 50 requests/second with HTTP, and presumably it would be about the same with WebSockets.  Even though it would be much less data transmitted with WebSockets, it would be the same number of packets, the same latency, etc.




