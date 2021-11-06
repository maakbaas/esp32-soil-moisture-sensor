# ESP32 Soil Moisture Sensor
Software and Gerber files for a simple soil moisture sensor using the ESP32 and a bistable status indicator.

More information on the project:
* https://maakbaas.com/esp32-soil-moisture-sensor/logs/logging-to-the-cloud/
* https://hackaday.com/2021/10/28/soil-sensor-shows-flip-dots-arent-just-for-signs/

## Known issues:

* The sensor functionality degraded for me after a few months in use. My hypothesis is that this is due to the PCB material soaking up moisture over time. If this hypothesis is correct, one way to prevent it would be to add epoxy around the PCB edges
* By mistake there is a ground plane underneath the ESP32 Antenna, this should be removed for better WiFi performance
* The code does not do well if there are WiFi connectivity issues. It will try to reconnect until the battery is drained
* Maximum real world battery life I have seen is around 4 months, but solving the previous point might lengthen that

## Photo:

![PCB](https://cdn.hackaday.io/images/6948091618431806487.jpg)

