from .serial_cli import *

class CanCrusher:
  def __init__(self, serial_device, user_interface):
    self.serial_device = serial_device
    self.user_interface = user_interface

  def run(self):
    self.user_interface.notify("INIT")
    self.cli = SerialCLI(self.serial_device)
    self.cli.kick()
    self.user_interface.notify("HOME")
    self.cli.wake()
#    self.cli.home()
    self.cli.sleep()

    while 1:
      msg = self.user_interface.request()
      if msg == "ACT":
        self.can_find()
        self.can_crush()
        self.can_release()
        
      else:
        raise RuntimeError("BAD MESSAGE")

  def can_find(self):
    self.user_interface.notify("FIND_CAN")
    found_can = False
  
    self.cli.sleep()
    self.cli.wake()
  
    try:
      self.cli.move(-200, 10)
    except SerialException:
      found_can = True
    
    if not found_can:
      raise RuntimeError("NO CAN")

    can_size = float(self.cli.position())
    print("FOUND CAN AT %f" % can_size)
  
  def can_crush(self):
    self.user_interface.notify("CRUSH_CAN")
    bad_count = 0
    print("SETTING UP TO CRUSH")
  
    self.cli.sleep()
    self.cli.wake()

    print("STARTING 3 mm per second")
  
    for i in range(0,10):
      self.cli.move(5, 3)
      try:
        self.cli.move(-10, 3)
      except SerialException:
        bad_count += 1
        print("X")

    print("FINISHED 3 mm per second")
#    print("STARTING 5 mm per second")
  
#    for i in range(0,1):
#      self.cli.move(5, 10)
#      try:
#        self.cli.move(-10, 5)
#      except SerialException:
#        bad_count += 1
#        print("X")

#    print("FINISHED 5 mm per second")
#    threshold = int(self.cli.get_prop("STALLGUARD_THRESHOLD"))
#    self.cli.set_prop("STALLGUARD_THRESHOLD", threshold - 10)
#    self.cli.sleep()
#    self.cli.wake()
#
#    print("STARTING 10 mm per second")
#    for i in range(0,5):
#      self.cli.move(10, 10)
#      try:
#        self.cli.move(-15, 10)
#      except SerialException:
#        bad_count += 1
#        print("Y")

#    print("FINISHED 10 mm per second")
      
  def can_release(self):
    self.user_interface.notify("RELEASE_CAN")
    self.cli.reset_props()
    self.cli.sleep()
    self.cli.wake()
    
    self.cli.move(150, 10)

  def can_full_crush(self):
    original_threshold = int(self.cli.get_prop("STALLGUARD_THRESHOLD"))
    try:
      self.can_find()
      self.can_crush()
      self.can_release()
    finally:
      self.cli.set_prop("STALLGUARD_THRESHOLD", original_threshold)
      self.cli.sleep()
