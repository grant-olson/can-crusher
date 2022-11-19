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

  def __init__(self, calibration=None):
    self.calibration = calibration

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
            # These are backwards on our screen, so swap in software
            self.clicks.append( (self.current_y, self.current_x, self.current_pressure) )
            print("--- %d %d %d" % (self.current_y, self.current_x, self.current_pressure))
            
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

  def get_button_click_scaled(self):
    if self.calibration is None:
      raise RuntimeError("Can't get scaled coordinates without calibration!")

    click = self.get_button_click()
    if click is None:
      return None
    
    click_x, click_y, _ = click
    x = (click_x - self.calibration["x_zero"]) / self.calibration["sfh"]
    y = (click_y - self.calibration["y_zero"]) / self.calibration["sfv"]

    return (x,y)
    
    
  def clear_clicks(self):
    self.clicks = []
    
class TouchScreenCalibrator():
  """
  Quick calibration algorithm. Assumes response is linear no
  curves. Gets several data samples then figures out how to 
  turn raw numbers in to a coordinate plane that matches 
  screen size and image orientation.

  This should be saved somewhere once and used by the touch
  screen instance later
  """
  def __init__(self):
    # defer loading since we normally DONT calibrate
    import can_crusher.spi_display
    self.ts = TouchScreen()
    self.sd = can_crusher.spi_display.SpiDisplay()
  
  def draw_plus(self, draw,x,y,color=(255,255,255)):
    draw.line((x,y-5,x,y+5), fill=color)
    draw.line((x-5,y,x+5,y), fill=color)

  def draw_calibration_image(self):
    from PIL import Image, ImageDraw

    cal_image = Image.new(mode="RGB", size=(320,240))
    draw = ImageDraw.Draw(cal_image)

    self.draw_plus(draw,20,20)
    self.draw_plus(draw,160,20)
    self.draw_plus(draw,300,20)
    self.draw_plus(draw,20,220)
    self.draw_plus(draw,160,220)
    self.draw_plus(draw,300,220)
    self.draw_plus(draw,20,120)
    self.draw_plus(draw,300,120)

    self.draw_plus(draw,160,120,color=(255,0,0))

    self.sd.display_pil_image(cal_image)


  def get_click(self, prompt):
    self.ts.clear_clicks()
    print(prompt)
    click = self.ts.get_button_click()
    while click is None:
      click = self.ts.get_button_click()
    return click

  def get_calibration(self):
    top_left_x, top_left_y, _ = self.get_click("TOUCH TOP LEFT")
    top_mid_x, top_mid_y, _ = self.get_click("TOUCH TOP MID")
    top_right_x, top_right_y, _ = self.get_click("TOUCH TOP RIGHT")
    mid_left_x, mid_left_y, _ = self.get_click("TOUCH MID LEFT")
    mid_right_x, mid_right_y, _ = self.get_click("TOUCH_MID_RIGHT")
    bottom_left_x, bottom_left_y, _ = self.get_click("TOUCH BOTTOM LEFT")
    bottom_mid_x, bottom_mid_y, _ = self.get_click("TOUCH BOTTOM MID")
    bottom_right_x, bottom_right_y, _ = self.get_click("TOUCH BOTTOM RIGHT")

    left = (top_left_x + mid_left_x + bottom_left_x) / 3
    right = (top_right_x + mid_right_x + bottom_right_x) / 3
    top = (top_left_y + top_mid_y + top_right_y) / 3
    bottom = (bottom_left_y + bottom_mid_y + bottom_right_y) / 3

    scaling_factor_horizontal = float(right-left) / 300.0
    scaling_factor_vertical = float(bottom-top) / 200.0

    print("SFH: %f SFV: %f" % (scaling_factor_horizontal, scaling_factor_vertical))

    zero_x = left - (20 * scaling_factor_horizontal)
    zero_y = top - (20 * scaling_factor_vertical)

    print("ZERO_X: %f, ZERO_Y: %f" % (zero_x, zero_y))

    return {"sfh": scaling_factor_horizontal, "sfv": scaling_factor_vertical,
      "x_zero": zero_x, "y_zero": zero_y}

  def calibrate(self):
    self.sd.test_grayscale()
    self.draw_calibration_image()
    calibration = self.get_calibration()

    click=self.get_click("TEST CENTER CLICK")
    center_x, center_y, _ = click

    x = (center_x - calibration["x_zero"]) / calibration["sfh"]
    y = (center_y - calibration["y_zero"]) / calibration["sfv"]

    print("RESULTS %f %f" % (x,y))

    return calibration
  
if __name__ == "__main__":
  calibrator = TouchScreenCalibrator()
  calib = calibrator.calibrate()
  
  ts = TouchScreen(calibration=calib)
  while (1):
    press = ts.get_button_click_scaled()
    if press is not None:
      print("CLICKED %s" % repr(press))
