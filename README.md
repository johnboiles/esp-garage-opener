*DEPRECATED. I've now moved to using [ESPHome](https://esphome.io/) for my garage door controller. See [here](https://github.com/johnboiles/homeassistant-config/tree/master/esphome) for my ESPHome configs (specifically look at [`garage_door.yaml`](https://github.com/johnboiles/homeassistant-config/blob/master/esphome/garage_door.yaml).*

# esp-garage-opener

Control and monitor your garage door over MQTT with Home Assistant with an ESP8266, a relay, and a magnetic reed switch.

![Open with Siri](https://raw.githubusercontent.com/johnboiles/esp-garage-opener/assets/images/siri-small.gif)
![Open with a button](https://raw.githubusercontent.com/johnboiles/esp-garage-opener/assets/images/button-small.gif)

Uses:
* [NodeMCU 1.0](https://www.amazon.com/HiLetgo-Version-NodeMCU-Internet-Development/dp/B010O1G1ES) ESP8266 Wifi microcontroller (though I'd probably use a [Wemos D1 Mini](https://www.amazon.com/Makerfocus-NodeMcu-Development-ESP8266-ESP-12F/dp/B01N3P763C) if I started over.
* [5V relay](https://www.amazon.com/Tolako-Arduino-Indicator-Channel-Official/dp/B00VRUAHLE) to trigger the door
* [Magnetic reed switch](https://www.amazon.com/uxcell-Window-Sensor-Magnetic-Recessed/dp/B00HR8CT8E) to detect whether the door is open or closed.
* MQTT protocol for communication
* [HomeAssistant](https://home-assistant.io/) and [Homebridge](https://github.com/nfarina/homebridge) running on a [Raspberry Pi 3](https://www.amazon.com/CanaKit-Raspberry-Micro-Supply-Listed/dp/B01C6FFNY4) to make it available to HomeKit (Siri)

## Electronics

The electronics are very simple. The relay is switched by a digital output from the microcontroller (the relay board linked above has a transistor to supply the necessary current to the relay). The reed switch is connected to ground and one of the digital inputs (because the ESP8266 has internal pullups, no extra resistor is necessary). Just connect to the normally disconnected side of your relay to the same two wires running to your existing garage door button.

![Fritzing diagram](https://raw.githubusercontent.com/johnboiles/esp-garage-opener/assets/images/fritzing.png)

## Compiling the code

First off you'll need to create a `src/secrets.h`. This file is `.gitignore`'d so you don't put your passwords on Github.

    cp src/secrets.example.h src/secrets.h

Then edit your `src/secrets.h` file to reflect your wifi ssid/password and Home Assistant password.

The easiest way to build and upload the code is with the [PlatformIO IDE](http://platformio.org/platformio-ide).

The first time you program your board you'll want to do it over USB. After that, programming can be done over wifi. To program over USB, change the `upload_port` in the `platformio.ini` file to point to the USB-serial device for your board. Probably something like the following will work if you're on a Mac.

    upload_port = /dev/tty.cu*

If you're not using the NodeMCU board, you'll also want to update the `board` line with your board. See [here](http://docs.platformio.org/en/latest/platforms/espressif8266.html) for other PlatformIO supported ESP8266 board. For example, for the Wemos D1 Mini:

    board = d1_mini

After that, from the PlatformIO Atom IDE, you should be able to go to PlatformIO->Upload in the menu.

## Configuring [Home Assistant](https://home-assistant.io/)

Add the following to your Home Assistant `configuration.yaml` file

```yaml
cover:
  - platform: mqtt
    name: Garage Door
    friendly_name: Garage
    state_topic: "garage/door"
    command_topic: "garage/button"
    payload_open: "OPEN"
    payload_close: "OPEN"
    payload_stop: "OPEN"
    state_open: "OPENED"
    state_closed: "CLOSED"
    optimistic: false
    retain: false
    value_template: '{{ value }}'
```

## Thanks / Attribution

* Thanks to [automatedhome.party's blog post](http://automatedhome.party/2017/01/06/wifi-garage-door-controller-using-a-wemos-d1-mini/) about building something like this. This repo is largely based on that blog post.
