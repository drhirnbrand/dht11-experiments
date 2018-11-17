# dht11-experiments
Playing around with a Raspberry Pi B (1 core 700Mhz) and a DHT11 Temperature/Humidity Sensor from an Arduino Experimentation Box

## Python

The first attempt using the Adafruit Python DHT library did not work out.
https://github.com/adafruit/Adafruit_Python_DHT

It is my assumption, that the old Raspi is just too slow.
I tried to code my own loop in pure python, and the edge-detection loop manages to finish 1-3 cycles before detecting a signal change.
So just a little delay and an edge is lost. That happens all the time except once every 10-15 tries.
Therefore it is not very stable. A Raspberry Pi 3 B+ should be much faster.

## C

After I got the Python part at least working partially,
I tried the same with a quick'n'dirty C program using pigpio
http://abyz.me.uk/rpi/pigpio/index.html

I am not a serious C coder, but I managed to cargo cult some working code together.
A edge-detection loop runs about 250 cycles in C.

