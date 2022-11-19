import RPi.GPIO as GPIO
import spidev
from time import sleep
from PIL import Image


class SpiDisplay:
  """
  A Spi Display. For now we're assuming it's a 320x240 display 
  and it's driven by an ILI9341 driver, and running on a Raspberry Pi.
  
  Since the default settings will conflict with the touch screen we want
  to use, we're also running it on SPI1. This is not enabled by default.
  To enable it add the following to your /boot/config.txt:

  dtoverlay=spi1-1cs
  
  In addition to that we have a reset pin on GPIO 5 and a D/C pin on GPIO 6.
  """

  def __init__(self):
    self.reset_pin = 5
    self.dc_pin = 6
    self.init_gpio()
    self.init_spi()
    self.init_display()
    
  def init_gpio(self):
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(self.reset_pin, GPIO.OUT)
    GPIO.setup(self.dc_pin, GPIO.OUT)

    # Run a hard reset
    GPIO.output(self.reset_pin, 0)
    GPIO.output(self.dc_pin, 0)
    GPIO.output(self.reset_pin, 1)
    GPIO.output(self.reset_pin, 0)
    GPIO.output(self.reset_pin, 1)
    
  def init_spi(self):
    self.spi = spidev.SpiDev()
    self.spi.open(1,0)

    self.spi.max_speed_hz = 64000000
    
  def init_display(self):

    # software reset
    self.send_cmd(0x01)
    sleep(0.1)

    # out of sleep
    self.send_cmd(0x11)

    # Important settings
    self.send_cmd(0x3a,0x55) # set pixel format to 5-6-5
    self.send_cmd(0x20) # invert display (21h) or not (20h)
    self.send_cmd(0x13) # normal display mode
    self.send_cmd(0x26, 1) # gamma set

    self.send_cmd(0x51, 0xFF) # full bright

    self.send_cmd(0x36, 12) # MADCTL

    self.send_cmd(0x29) # display ON

  def send_cmd(self, *args):
    GPIO.output(self.dc_pin,0)
    self.spi.xfer([args[0]])
    GPIO.output(self.dc_pin,1)
    if len(args) > 1:
      self.spi.xfer(args[1:])
      
  def prep_data(self):
    self.send_cmd(0x2c)
    GPIO.output(self.dc_pin, 1)
    
  def test_grayscale(self):
    self.prep_data()
    print("Trying White Gradient")

    for i in range(0,360):
      val = i // 32
      val *= 32
      gradient_percent = float(val) / 320.0
      blue_max_value = 31.0
      green_max_value = 63.0
      gradient = int(blue_max_value * gradient_percent)
      green_gradient = int(green_max_value * gradient_percent)
      g_msb = green_gradient >> 3
      g_lsb = (green_gradient & 0x7) << 5
      byte_one = (gradient << 3) + g_msb
      byte_two = gradient + g_lsb 
      data = [byte_one, byte_two] * 240
      self.spi.xfer(data)

  def test_random(self):
    self.prep_data()

    print("TRYING RANDOM")

    import random
    for i in range(0,320*240*2//4096):
      self.spi.xfer(random.randbytes(4096))

  def test_primary_and_secondary(self):
    self.prep_data()
    print("TRYING COLORS Red, Green, Blue>, ")

    red = 31
    green_1 = 1+2+4
    green_2 = 128+64+32
    blue = 128+64+32+16+8

    for i in range(0,320):
      stuff = [0, red]
      stuff *= 40
      self.spi.xfer(stuff)
    for i in range(0,320):
      stuff = [green_1, green_2]
      stuff *= 40
      self.spi.xfer(stuff)
    for i in range(0,320):
      stuff = [blue,0]
      stuff *= 40
      self.spi.xfer(stuff)

    # Should be yellow
    for i in range(0,320):
      stuff = [green_1, green_2+red]
      stuff *= 40
      self.spi.xfer(stuff)

    # should be cyan
    for i in range(0,320):
      stuff = [blue+green_1, green_2]
      stuff *= 40
      self.spi.xfer(stuff)
    # should be magenta
    for i in range(0,320):
      stuff = [blue,red]
      stuff *= 40
      self.spi.xfer(stuff)
  

  def test_red_gradient(self):
    self.prep_data()
    print("TRYING GRADIENT RED")

    for i in range(0,320):
      gradient_percent = float(i) / 320.0
      red_max_value = 31.0
      gradient = int(red_max_value * gradient_percent)
      data = [gradient << 3,0] * 240
      self.spi.xfer(data)

  def test_blue_gradient(self):
    self.prep_data()
    print("TRYING GRADIENT BLUE")

    for i in range(0,320):
      gradient_percent = float(i) / 320.0
      blue_max_value = 31.0
      gradient = int(blue_max_value * gradient_percent)
      data = [0,gradient] * 240
      self.spi.xfer(data)

  def slide_iterator(self):
    import glob
    while 1:
      for file_name in glob.glob("*.raw565"):

        f = open(file_name,"rb")

        self.prep_data()

        data = f.read(4096)
        while len(data) > 0:
          self.spi.xfer(data)
          data = f.read(4096)
        yield

  def slideshow(self):
    slides = self.slide_iterator()
    
    for i in self.slide_iterator():
      next(slides)
      sleep(10)

  
  def test_pattern(self):
    interval = 0.25
    for test in [self.test_random, self.test_grayscale, self.test_primary_and_secondary,
              self.test_red_gradient, self.test_blue_gradient]:
      sleep(interval)
      test()

  def rgb_to_565(self,r,g,b):
    r = (r >> 3) << 3 # make 5 bits, packed to left
    b = (b >> 3) # five bits, packed to right.
    g = (g >> 2) # 6 bits

    g1 = g >> 3 # top 3
    g2 = (g & 0x7) << 5 # bottom 3 bits shifted to left.

    byte1 = r + g1
    byte2 = g2 + b

    return (byte1, byte2)
    
  def display_pil_image(self,img):
    """
    Put a 320,240 PIL Image object on the screen.

    This is way too slow for normal use, but helpful for
    dev tasks and debugging.
    """
    self.prep_data()

    for x in range(0,img.width):
      for y in range(0,img.height):
        pixel = img.getpixel( (x,y) )
        r = pixel[0]
        g = pixel[1]
        b = pixel[2]
        bytes = self.rgb_to_565(r,g,b)
        self.spi.xfer(bytes)
        
if __name__ == "__main__":
  spi_display = SpiDisplay()
  spi_display.test_pattern()
  spi_display.slideshow()
