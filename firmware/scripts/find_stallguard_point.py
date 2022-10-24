#!/usr/bin/env python

from serial_cli import *
  
cli = SerialCLI("/dev/ttyUSB0")

cli.sleep()
cli.version()
print(cli.get_prop("STALLGUARD_THRESHOLD"))
cli.cmd("VSDF")

def narrow_sg_range(bad, good, step, speed):
  for i in range(bad, good, step):
    cli.set_prop("STALLGUARD_THRESHOLD", i)
    cli.sleep()
    cli.wake()
    try:
      cli.move(-10, speed)
      cli.move(10, speed)
      return (i - step, i)
    except SerialException as ex:
      pass # Later check for stall

  print("GOT TO END")
  print(i)
  return (i,0)


def find_range(speed):
  bad, good = narrow_sg_range(255,0,-16, speed)
  bad, good = narrow_sg_range(bad, good,-4, speed)
  bad, good = narrow_sg_range(bad, good,-1, speed)

  print("SPEED: %d BAD %d, GOOD %d" % (speed, bad, good))
#  cli.move(100, speed)
#  cli.move(-100, speed)
  


#find_range(5)
find_range(10)
find_range(15)
find_range(20)
find_range(25)
find_range(30)
find_range(35)
