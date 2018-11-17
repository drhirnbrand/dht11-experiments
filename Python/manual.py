#!/usr/bin/python

import cython
import sys
import RPi.GPIO as GPIO
import array
import time
import psutil, os

pid = psutil.Process(os.getpid())
pid.nice(-20)

DHT_PULSES = 41
DHT_MAXCOUNT = 32000

pulseCounts = [0] * (DHT_PULSES*2)

pin=7

GPIO.setmode(GPIO.BOARD)

pulses=xrange(0, (DHT_PULSES) * 2, 2)
loops=xrange(0,32)

t=0

# Init sequence
GPIO.setup(pin, GPIO.OUT)
GPIO.output(pin,GPIO.HIGH)
time.sleep(750/1000)    # Interface settle down

GPIO.output(pin,GPIO.LOW)
time.sleep(12/1000)     # at least 18ms Low


GPIO.setup(pin, GPIO.IN, pull_up_down = GPIO.PUD_UP)

# Waiting for PIN to go LOW

while GPIO.input(pin) == GPIO.HIGH:
    if t > 10000:
        print('Timeout')
        sys.exit(1)
    t = t + 1


# Detector Loop


for i in pulses:

    # Count LOW cycles
    for lc in loops:
        if GPIO.input(pin) == GPIO.HIGH:
            break

    pulseCounts[i] = lc

    for hc in loops:
        if GPIO.input(pin) == GPIO.LOW:
            break

    pulseCounts[i+1] = hc

for i in range(0, DHT_PULSES * 2, 2):
    print('Bit {}: {} HIGH, {} LOW'.format(i/2, pulseCounts[i], pulseCounts[i+1]))



pulseSum = 0
for i in xrange(2, DHT_PULSES * 2, 2):
    pulseSum = pulseSum + pulseCounts[i]
pulseWidth = pulseSum / (DHT_PULSES - 1)

print('Reference 1 = {} c'.format(pulseWidth))

data = [0] * 5

for i in xrange(2, DHT_PULSES * 2, 2):
    index = (i - 2) / (8 * 2)
    data[index] = data[index] << 1
    if pulseCounts[i + 1] >= pulseWidth:
        data[index] = data[index] | 1

print('Data {} {} {} {}'.format(data[0], data[1], data[2], data[3], data[4]))
if data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xff):
    humidity = data[0]
    temperature = data[2]

    print('Humidity {}, Temperature {}'.format(humidity, temperature))
