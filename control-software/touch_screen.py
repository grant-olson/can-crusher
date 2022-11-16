import evdev

class TouchScreen:
  """
  This assumes we have a ads7846 compatible touch screen, and the pinouts
  assume we're on a Raspberry Pi. To use on a Raspberry Pi you'll need
  to add the following to /boot/config.txt to load the driver for the 
  touchscreen:

  dtoverlay=ads7846,cs=0,penirq=26,speed=10000,penirq_pull=2,xohms=150

  Run `dmesg` to make sure it was loaded correctly. In this case the 
  IRQ pin is 26 but it can be changed to fit your layout.
  """

  def __init__(self):
    devices = [evdev.InputDevice(path) for path in evdev.list_devices()]

    touchscreens = [device for device in devices if "Touchscreen" in device.name]

    touchscreen_count = len(touchscreens)
    if touchscreen_count != 1:
      raise RuntimeError("Expected 1 touchscreen, found %d" % touchscreen_count)

    self.touchscreen = touchscreens[0]

    self.current_x = -1
    self.current_y = -1
    self.current_pressure = -1

    self.clicks = []

  def get_events(self):
    try:
      return [x for x in self.touchscreen.read()]
    except BlockingIOError as ex:
      return []

  def process_events(self):
    events = self.get_events()
    if len(events) > 0:
      for event in events:
        value = event.value
        if event.type == evdev.ecodes.EV_ABS:
          if event.code == evdev.ecodes.ABS_X:
            self.current_x = value
          elif event.code == evdev.ecodes.ABS_Y:
            self.current_y = value
          elif event.code == evdev.ecodes.ABS_PRESSURE:
            self.current_pressure = value
        elif event.type == evdev.ecodes.EV_SYN:
          pass
        elif event.type == evdev.ecodes.EV_KEY and event.code == evdev.ecodes.BTN_TOUCH:
          if event.value == 0:
            self.clicks.append( (self.current_x, self.current_y, self.current_pressure) )
            print("--- %d %d %d" % (self.current_x, self.current_y, self.current_pressure))
            
            self.current_x = -1
            self.current_y = -1
            self.current_pressure = -1

        else:
          print("??? %s" % evdev.categorize(event))        

  def get_button_click(self):
    self.process_events()
    if len(self.clicks) == 0:
      return None
    else:
      next_click = self.clicks[0]
      self.clicks = self.clicks[1:]
      return next_click

  def clear_clicks(self):
    self.clicks = []
    
if __name__ == "__main__":
  ts = TouchScreen()
  while (1):
    press = ts.get_button_click()
    if press is not None:
      print("CLICKED %s" % repr(press))
