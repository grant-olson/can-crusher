#!/usr/bin/env python

from serial_cli import *
import statistics
from time import sleep
import sys

cli = SerialCLI("/dev/ttyUSB0")

def retry_wake():
  try:
    cli.wake()
  except SerialException:
    cli.wake()

def narrow_sg_range(bad, good, step, speed):
  sys.stdout.write("Trying ")
  for i in range(bad, good-1, step):
    cli.set_prop("STALLGUARD_THRESHOLD", i)
    cli.sleep()
    sleep(1)
    sys.stdout.write("%i. " % i)
    sys.stdout.flush()
    retry_wake()
    try:
      cli.move(-10, speed)
      cli.move(10, speed)
      print()
      return (i - step, i)
    except SerialException as ex:
      pass # Later check for stall
  print()
  print("Failed to find inflection point!")
  return (0,0)


def find_range_once(speed):
  bad, good = 255 , 0
  bad, good = narrow_sg_range(bad, good, -64, speed)
  if bad == 0: return 0
  bad, good = narrow_sg_range(bad, good, -16, speed)
  if bad == 0: return 0
#  bad, good = narrow_sg_range(bad, good,-8, speed)
#  if bad == 0: return 0
  last_good = good
  bad, good = narrow_sg_range(bad, good, -1, speed)

  # If we didn't get it on 1 we might be on the very edge of stall
  # detection. Try again along that range.
  
  if bad == 0:
    bad, good = narrow_sg_range(last_good+3, last_good-3, -1, speed)
  print("SPEED: %d BAD %d, GOOD %d" % (speed, bad, good))
  return good
  

def find_range(speed):
  results = [find_range_once(speed) for x in range(0,5)]
  print("RAW RESULTS: %s" % repr(results))
  results = [x for x in results if x != 0]
  if len(results) < 3:
    raise RuntimeError("BAD DATA POINTS!")
  average = statistics.mean(results)
  print("AVERAGE: %f" % average)
  safe_average = average * 0.95 # give an extra 5%
  safe_average = int(safe_average)
  print("FINAL: %d" % safe_average)

values = []

# 5 - 84
# 7 - 109
# 10 - 132
# 12 - 144
# 15 - 157
# 17 - 158
# 20 - 171
# 22 - 151
# 25 - 135 ?
# 30 - ???

for i in range(28,36,5):
  cli.set_prop("STALLGUARD_THRESHOLD", 171)
  cli.sleep()
  cli.wake()
  cli.move(10,20)
  cli.move(-10,20)

  res = find_range(i)
  values.append( (i,res) )

  ten_speed = 131 #values[0][1]
  cli.set_prop("STALLGUARD_THRESHOLD", ten_speed)
  cli.sleep()
  cli.wake()
  cli.home()
  sleep(1.0)
  cli.move(50, 10)
  
print(repr(values))
