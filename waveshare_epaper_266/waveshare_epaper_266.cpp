#include "waveshare_epaper_266.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace waveshare_epaper_266 {

static const char *const TAG = "waveshare_epaper.266";

void WaveshareEPaper266::setup() {
  const size_t buffer_size = this->get_width_internal() * this->get_height_internal() / 8u;
  this->init_internal_(buffer_size);
  
  if (this->buffer_ != nullptr) {
    memset(this->buffer_, 0xFF, buffer_size);
  }
  
  this->spi_setup();
  
  this->dc_pin_->setup();
  this->dc_pin_->digital_write(false);
  
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
  }
  
  if (this->busy_pin_ != nullptr) {
    this->busy_pin_->setup();
    this->busy_pin_->pin_mode(gpio::FLAG_INPUT);
  }
  
  // Hardware Reset
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->digital_write(true);
    delay(200);
    this->reset_pin_->digital_write(false);
    delay(2);
    this->reset_pin_->digital_write(true);
    delay(200);
  }
  
  this->wait_until_idle_();
  
  ESP_LOGD(TAG, "Init 2.66\" - exact Waveshare sequence");
  
  // Soft reset
  this->command(0x12);
  this->wait_until_idle_();
  
  // Data entry mode (Waveshare demo uses 0x03)
  this->command(0x11);
  this->data(0x03);
  
  // Set RamX-address Start/End
  // WIDTH=152, so (152%8==0) ? 152/8 : 152/8+1 = 19
  this->command(0x44);
  this->data(0x01);
  this->data(0x13);  // 19 = 0x13
  
  // Set RamY-address Start/End
  // HEIGHT=296 = 0x0128
  this->command(0x45);
  this->data(0x00);  // Start LSB
  this->data(0x00);  // Start MSB
  this->data(0x28);  // End LSB (296 & 0xFF = 0x28)
  this->data(0x01);  // End MSB ((296 & 0x100) >> 8 = 0x01)
  
  this->wait_until_idle_();
  
  ESP_LOGCONFIG(TAG, "Display initialized - no clear");
}

int WaveshareEPaper266::get_height_internal() {
  return 296;  // GEÄNDERT!
}

int WaveshareEPaper266::get_width_internal() {
  return 152;  // GEÄNDERT!
}

void WaveshareEPaper266::dump_config() {
  LOG_DISPLAY("", "Waveshare E-Paper", this);
  ESP_LOGCONFIG(TAG, "  Model: 2.66in (152x296)");
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  ESP_LOGCONFIG(TAG, "  Full Update Every: %u", this->full_update_every_);
  LOG_UPDATE_INTERVAL(this);
}

void WaveshareEPaper266::update() {
  this->do_update_();
  this->display();
}

void HOT WaveshareEPaper266::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || x < 0 || y >= this->get_height_internal() || y < 0)
    return;
  
  const uint32_t pos = (x + y * this->get_width_internal()) / 8u;
  const uint8_t subpos = x & 0x07;
  const uint8_t mask = (0x80 >> subpos);
  
  // Waveshare/EPD: 1 = WHITE, 0 = BLACK.
  // In ESPHome, COLOR_ON is the "on" color (default) and COLOR_OFF is "off".
  // We map: COLOR_ON -> WHITE (bit=1), COLOR_OFF -> BLACK (bit=0).
  if (color.is_on()) {
    this->buffer_[pos] |= mask;   // 1 = white
  } else {
    this->buffer_[pos] &= ~mask;  // 0 = black
  }
}

void WaveshareEPaper266::display() {
  ESP_LOGD(TAG, "Updating display with buffer...");
  
  const uint16_t width_bytes = 19;  // 152 / 8
  const uint16_t height = 296;
  
  // WICHTIG: RAM Counter zurücksetzen BEVOR wir Daten schreiben!
  this->command(0x4E);  // Set RAM X address counter
  this->data(0x01);     // Start bei 1 (nicht 0!)
  
  this->command(0x4F);  // Set RAM Y address counter
  // IMPORTANT: Start from the LAST line (HEIGHT-1). This matches Waveshare examples and
  // avoids writing the buffer "upside down" on some controllers.
  const uint16_t y = height - 1;  // 295 = 0x0127
  this->data(y & 0xFF);           // 0x27
  this->data((y >> 8) & 0xFF);    // 0x01
  
  // Send buffer to display
  this->command(0x24);
  this->start_data_();
  
  uint32_t addr = 0;
  for (uint16_t j = 0; j < height; j++) {
    const uint16_t src_row = (height - 1) - j;
    for (uint16_t i = 0; i < width_bytes; i++) {
      addr = i + src_row * width_bytes;
      this->write_buffer_(this->buffer_[addr]);
    }
  }
  this->end_data_();
  
  // Turn on display
  this->command(0x20);
  this->wait_until_idle_();
  
  ESP_LOGD(TAG, "Display update complete");
}

void WaveshareEPaper266::command(uint8_t value) {
  this->start_command_();
  this->write_buffer_(value);
  this->end_command_();
}

void WaveshareEPaper266::data(uint8_t value) {
  this->start_data_();
  this->write_buffer_(value);
  this->end_data_();
}

void WaveshareEPaper266::write_buffer_(uint8_t value) {
  this->write_byte(value);
}

void WaveshareEPaper266::start_command_() {
  this->dc_pin_->digital_write(false);
  this->enable();
}

void WaveshareEPaper266::end_command_() {
  this->disable();
}

void WaveshareEPaper266::start_data_() {
  this->dc_pin_->digital_write(true);
  this->enable();
}

void WaveshareEPaper266::end_data_() {
  this->disable();
}

bool WaveshareEPaper266::wait_until_idle_() {
  if (this->busy_pin_ == nullptr) {
    return true;
  }
  
  const uint32_t start = millis();
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > 20000u) {
      ESP_LOGE(TAG, "Timeout while waiting for busy signal");
      return false;
    }
    delay(100);
  }
  delay(100);
  return true;
}

void WaveshareEPaper266::reset_() {
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->digital_write(true);
    delay(200);
    this->reset_pin_->digital_write(false);
    delay(2);
    this->reset_pin_->digital_write(true);
    delay(200);
  }
}

void WaveshareEPaper266::deep_sleep() {
  this->command(0x10);
  this->data(0x01);
}

}  // namespace waveshare_epaper_266
}  // namespace esphome
