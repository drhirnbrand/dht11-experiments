#!/usr/bin/python

import sys
import RPi.GPIO as GPIO
import Adafruit_DHT

GPIO.setmode(GPIO.BOARD)
pin=7

humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.DHT11, pin)

if humidity is not None and temperature is not None:
    print('Temp={0:0.1f}*  Humidity={1:0.1f}%'.format(temperature, humidity))
else:
    print('Failed to get reading. Try again!')
    sys.exit(1)
