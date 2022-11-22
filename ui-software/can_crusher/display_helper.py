from PIL import Image, ImageFont, ImageDraw
from glob import glob
import random
import os


class DisplayHelper:
  """
  A Bunch of helper values and routines for the SPI display.

  We're currently running the display on a Raspberry Pi Zero with
  a small touchscreen display. It's a little unusual because the Pi
  is a little underpowered by desktop standards, but unlike an embedded
  system has Gigs of available space on the SD Card.

  So we pre-render all the screen tiles ahead of time and save them 
  in a raw format that can later be copied directly to the SpiDisplay
  to get a reasonable framerate. This is a very NON general purpose set
  of code to accomplish this.

  The images are also saved as .pngs to make it easy to review.

  There are currently two sets of images:

  1) Slides while we are in 'kiosk' mode that display random information.
     These use a goofy way to randomize the slides. They are assigned
     a random number prefix when generated. Because of this we should
     purge old slides when generating new ones.

  2) Admin screens that show while we're doing work.
  """

  def __init__(self):
    self.height = 240
    self.width = 320

    # Not primary
    self.yellow = (0xFE, 0xCB, 0x0)
    self.white = (250,250,250)
    self.green = (0x22,0x8b,0x22)

    self.font_height = 24

    self.bold_font = ImageFont.truetype("LiberationSans-Bold.ttf",
                                        self.font_height)
    self.bold_italic_font = ImageFont.truetype("LiberationSans-BoldItalic.ttf",
                                               self.font_height)
    self.big_bold_font = ImageFont.truetype("LiberationSans-Bold.ttf",
                                            int(self.font_height*1.5))

    self.button_corners = [(0+40,self.height-36), (self.width-40,self.height-12)]
    
    # These are used to mix up the kiosk slide order
    # so they're shuffled vs the input list
    self.random_numbers = list(range(0,2000))
    random.shuffle(self.random_numbers)

    self.root_output_dir = "output"
    self.slide_prefix = "slides"
    self.admin_prefix = "admin"
    
  def get_random_number(self):
    return self.random_numbers.pop()

  def output_filename(self, group, file_format, sequence_number,
                      base_name, ext):
    if sequence_number is None:
      return ("%s/%s/%s/%s.%s" %
              (self.root_output_dir, group, file_format, base_name, ext))
    else:
      return ("%s/%s/%s/%04d_%s.%s" %
              (self.root_output_dir, group, file_format, sequence_number, base_name, ext))

  def draw_button(self, draw):
    draw.rectangle( self.button_corners, fill=self.green, outline=self.white)
    draw.text((45, 240-38), "INITIATE CRUSHING", self.white, font=self.bold_font)

  def save_raw_fb(self, image, filename):
    """
    Save raw in a format that can be copied directly to /dev/fb0
    """
    data = b''

    for j in range(0,self.height):
      for i in range(0,self.width):
        pixel = image.getpixel( (i,j) )
        r = pixel[0]
        g = pixel[1]
        b = pixel[2]
        data += b"%c%c%c%c" % (b,g,r,255)

    f = open(filename, "wb")
    f.write(data)
    f.close()
  
  def save_raw_565(self, image, filename):
    data = b''

    for i in range(0,self.width):
      for j in range(0,self.height):
        pixel = image.getpixel( (i,j) )
        r = pixel[0]
        g = pixel[1]
        b = pixel[2]

        r = (r >> 3) << 3 # make 5 bits, packed to left
        b = (b >> 3) # five bits, packed to right.
        g = (g >> 2) # 6 bits
        g1 = g >> 3 # top 3
        g2 = (g & 0x7) << 5 # bottom 3 bits shifted to left.

        byte1 = r + g1
        byte2 = g2 + b
      
        data += b"%c%c" % (byte1, byte2)

    f = open(filename, "wb")
    f.write(data)
    f.close()


  def add_text(self, image, text, font, color, position):
    current_x, current_y = position
    space_size, space_height = image.textsize(" ", font=font)
  
    words = text.split(" ")
    for word in words:
      next_word_length, next_word_height = image.textsize(word, font=font)
      if (current_x + next_word_length) >= 320:
        current_y += space_height
        current_x = 0
      image.text((current_x, current_y), word, color, font=font)
      current_x += next_word_length
      current_x += space_size

    if current_x != 0:
      current_y += self.font_height
      current_x = 0
    
    return (current_x, current_y)

  def save_variants(self, group, image, filename, random_id=None):
          
    image.save(self.output_filename(group, "png",random_id, filename, "png"))
    #save_raw_fb(image,output_filename(self, group, "raw_fb", random_id, filename, "raw"))
    self.save_raw_565(image,self.output_filename(group, "raw_565", random_id, filename, "565"))

  def make_fact_slide(self, background_image, title_text, main_text):

    image = Image.open(background_image)

    position = (0,0)
    modified_image = ImageDraw.Draw(image)

    position = self.add_text(modified_image, title_text, self.bold_italic_font,
                        self.yellow, position)
    self.add_text(modified_image, main_text, self.bold_font, self.white,
                  position) 
  
    self.draw_button(modified_image)

    random_id = self.get_random_number()

    self.save_variants(self.slide_prefix, image, "facts", random_id)
  
  def make_attack_slide(self, attack_text):

    image = Image.new("RGB", (320,240), color=(0xFF,0xA5, 0))

    position = (0,0)
    modified_image = ImageDraw.Draw(image)

    self.add_text(modified_image, attack_text, self.big_bold_font,
                  self.white, position) 
  
    random_id = self.get_random_number()

    self.save_variants(self.slide_prefix, image, "attack_ads", random_id)
  
  def make_trivia_slide(self, background_image, trivia):
    image = Image.open(background_image)

    position = (0,0)
    modified_image = ImageDraw.Draw(image)

    position = self.add_text(modified_image, "Trivia!", self.bold_italic_font,
                             self.yellow, position)
    position = self.add_text(modified_image, "Q: " + trivia[0], self.bold_font,
                             self.white, position) 
  
    self.draw_button(modified_image)

    random_id = self.get_random_number()

    self.save_variants(self.slide_prefix, image, "1_trivia", random_id)

    position = self.add_text(modified_image, "A: " + trivia[1], self.bold_font,
                             self.yellow, position) 

    self.save_variants("slides", image, "2_trivia", random_id)

  def make_admin_slide(self, cmd, msg):
    image = Image.new("RGB", (320,240), color=(0, 0, 0))
    position = (0,24)
    modified_image = ImageDraw.Draw(image)

    position = self.add_text(modified_image, msg, self.big_bold_font,
                             self.white, position)

    self.save_variants(self.admin_prefix, image, cmd)

  def make_directory_skeleton(self):
    for dir in ["slides", "admin"]:
      for subdir in ["png", "raw_fb", "raw_565"]:
        full_dir = "%s/%s/%s" % (self.root_output_dir, dir, subdir)
        if not os.path.exists(full_dir):
          os.makedirs(full_dir)

  def get_existing_slide_files(self):
    pngs = glob("%s/%s/png/*.png" % (self.root_output_dir, self.slide_prefix))
    raw_565s = glob("%s/%s/raw_565/*.565" % (self.root_output_dir, self.slide_prefix))
    return pngs + raw_565s  
          
  def old_slide_check(self):
    
    if len(self.get_existing_slide_files()) > 0:
      raise RuntimeError("We have old slides! These must be deleted with --purge")

  def purge_old_slides(self):
    old_slides = self.get_existing_slide_files()

    for slide_file in old_slides:
      print("DELETING %s..." % slide_file)
      os.remove(slide_file)
      
  def slide_iterator(self):
    import glob
    while 1:
      slides = glob.glob("%s/%s/raw_565/*.565" %
                         (self.root_output_dir, self.slide_prefix))
      if len(slides) == 0:
        raise RuntimeError("No pregenerated slides. Did you run ./slide-maker?")
      slides.sort()
      for file_name in slides:
        yield file_name

  def get_admin_screen_file_name(self, cmd):
    file_name = "%s/%s/raw_565/%s.565" % (self.root_output_dir,
                                          self.admin_prefix, cmd)

    if not os.path.exists(file_name):
      file_name = "%s/%s/raw_565/UNKNOWN.565" %(self.root_output_dir,
                                                self.admin_prefix)

    return file_name
