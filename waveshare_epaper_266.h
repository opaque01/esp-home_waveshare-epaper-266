#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace waveshare_epaper_266 {

class WaveshareEPaper266 : public display::DisplayBuffer,
                            public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                                   spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_4MHZ> {
 public:
  void set_dc_pin(GPIOPin *dc_pin) { dc_pin_ = dc_pin; }
  void set_reset_pin(GPIOPin *reset_pin) { reset_pin_ = reset_pin; }
  void set_busy_pin(GPIOPin *busy_pin) { busy_pin_ = busy_pin; }
  void set_full_update_every(uint32_t full_update_every) { 
    this->full_update_every_ = full_update_every; 
  }

  void setup() override;
  void dump_config() override;
  void update() override;
  
  display::DisplayType get_display_type() override { 
    return display::DisplayType::DISPLAY_TYPE_BINARY; 
  }

  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_height_internal() override;
  int get_width_internal() override;
  
  void display();
  void deep_sleep();
  
  void command(uint8_t value);
  void data(uint8_t value);
  void write_buffer_(uint8_t value);
  void start_command_();
  void end_command_();
  void start_data_();
  void end_data_();
  
  bool wait_until_idle_();
  void reset_();

  GPIOPin *dc_pin_;
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};
  uint32_t full_update_every_{30};
  uint32_t at_update_{0};
};

}  // namespace waveshare_epaper_266
}  // namespace esphome