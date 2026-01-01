# waveshare_epaper_266
ESP Home Waveshare 2,66 inch e-ink component

location: /homeassistant/esphome/my_components/waveshare_epaper_266


i2c:

  sda: D2

  scl: D1

  scan: true
  
  id: bus_a

spi:
  
  clk_pin: D5
  
  mosi_pin: D7


display: 
  
  cs_pin: D8
  
  dc_pin: D3
  
  busy_pin: D6
  
  reset_pin: D0
