from PIL import Image, ImageFont, ImageDraw
from glob import glob

screen_height = 240
screen_width = 320

yellow = (0xFE, 0xCB, 0x0)
white = (250,250,250)
green = (0x22,0x8b,0x22)
height = 24

bold_font = ImageFont.truetype("LiberationSans-Bold.ttf", height)
bold_italic_font = ImageFont.truetype("LiberationSans-BoldItalic.ttf", height)

facts = [
    "That British people PRONOUNCE aluminum differently because they SPELL it differently?",
    "Napolean Bonaparte treasured his rare aluminum-ware, and he forced his guests to eat with lowly gold-ware?",
    "When alien archeologists discover a dead Earth, that pure Aluminum will be the primary marker of the Anthropocene Era?",
    "Before Development of the BESSEMER PROCESS pure Aluminum was extremely rare?",
    "Aluminum has an Atomic Number of 13?",
    "Aluminum based antiperspirant works by clogging your sweat pores and may give you ALZHEIMERS?"
    ]


ads = [
    "Hershell Walker is WRONG for GEORGIA and the SENATE.",
    "Reverend Warnock's EXTREME SOCIALISM and DANGEROUS AGENDA are TOO MUCH for GEORGIA"
]

def draw_button(draw):
  draw.rectangle( [(0+40,240-36), (320-40,240-12)], fill=green, outline=white)
  draw.text((45, 240-38), "INITIATE CRUSHING", white, font=bold_font)

def save_raw_fb(image, name):
  """
  Save raw in a format that can be copied directly to /dev/fb0
  """
  data = b''

  for j in range(0,screen_height):
    for i in range(0,screen_width):
      pixel = image.getpixel( (i,j) )
      r = pixel[0]
      g = pixel[1]
      b = pixel[2]
      data += b"%c%c%c%c" % (b,g,r,255)

  f = open("./output/raw_fb/"+name+".raw", "wb")
  f.write(data)
  f.close()
  
def save_raw_565(image, name):
  data = b''

  for i in range(0,screen_width):
    for j in range(0,screen_height):
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

  f = open("./output/raw_565/"+name+".raw565", "wb")
  f.write(data)
  f.close()
  
def make_slide(background_image, title_text, main_text, output_image):

  image = Image.open(background_image)

  modified_image = ImageDraw.Draw(image)

  modified_image.text((0,0), title_text, yellow, font=bold_italic_font)
    
  current_x, current_y = 0, height * 1

  space_size = modified_image.textsize(" ", font=bold_font)[0]

  words = main_text.split(" ")
  for word in words:
    next_word_length, next_word_height = modified_image.textsize(word, font=bold_font)
    if (current_x + next_word_length) >= 320:
      print("OVERFLOW AT %s" % word)
      current_y += height
      current_x = 0
    modified_image.text((current_x, current_y), word, white, font=bold_font)
    current_x += next_word_length
    current_x += space_size

  draw_button(modified_image)
  
  image.save("./output/png/"+output_image)
  save_raw_fb(image,output_image)
  save_raw_565(image,output_image)
  
fact_backgrounds = glob("./background_img/*")
  
for i, text in enumerate(facts):
    make_slide(fact_backgrounds[i%len(fact_backgrounds)], "DID YOU KNOW...", facts[i], "fact_%d.png" % i)
