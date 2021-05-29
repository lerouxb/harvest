Harvest
===

A super low power energy-harvesting wifi temperature, humidity and voltage logger plus timelapse camera.

Very much a work in progress.

See https://lerouxb.co/2021/05/13/energy-harvesting-wifi.html for more details.

tl;dr
---

5.5V solar panel charges a 5.5V or 6V multi-farad supercapacitor. An attiny 1614 (could be smaller flash or pin count, could be 0 series as well - this is just what I had on hand) is connected straight to the supercapacitor. It measures its own supply voltage. If that is > 3.6V it powers a esp32 cam through a p-channel mosfet and 350mv dropout linear regulator. The esp32 cam reads a DHT22 to get temperature and humidity and asks the attiny for the supply voltage over i2c. It then connects to wifi and publishes those three values to an MQTT broker. Then it asks the attiny over i2c to cut the power. The attiny sleeps almost all the time. It just wakes up every second from the RTC and also when its i2c address matches. If 10 seconds passes without the esp32 signalling that it is done it will also cut the power as a failsafe just in case it is stuck. Every 60 seconds if the supply voltage is high enough it powers up the esp32 again and the cycle starts over.

Why an esp32 cam? So I can also use it to periodically take pictures and assemble them into a timelapse on the server.

Since the attiny is running at nominally 5V-ish and the esp32 at a regulated 3.3V, there's a [level shifter](https://cdn-shop.adafruit.com/datasheets/an97055.pdf) for the i2c bus.

The server also has an MQTT subscriber that stores the values in a mongo database that we can then pull out and graph.

An obvious optimisation would be to not even bother turning the esp32 on or taking pictures after sunset and before sunrise.
