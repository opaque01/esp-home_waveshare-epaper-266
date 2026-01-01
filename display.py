import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display, spi
from esphome.const import (
    CONF_BUSY_PIN,
    CONF_DC_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_RESET_PIN,
)

CONF_FULL_UPDATE_EVERY = "full_update_every"

CODEOWNERS = ["@custom"]
DEPENDENCIES = ["spi"]

waveshare_epaper_266_ns = cg.esphome_ns.namespace("waveshare_epaper_266")
WaveshareEPaper266 = waveshare_epaper_266_ns.class_(
    "WaveshareEPaper266",
    display.DisplayBuffer,
    spi.SPIDevice,
)

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(WaveshareEPaper266),
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_FULL_UPDATE_EVERY, default=1): cv.uint32_t,
        }
    )
    .extend(spi.spi_device_schema()),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    
    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    dc = await cg.gpio_pin_expression(config[CONF_DC_PIN])
    cg.add(var.set_dc_pin(dc))

    if CONF_RESET_PIN in config:
        reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset))

    if CONF_BUSY_PIN in config:
        busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
        cg.add(var.set_busy_pin(busy))
    
    if CONF_FULL_UPDATE_EVERY in config:
        cg.add(var.set_full_update_every(config[CONF_FULL_UPDATE_EVERY]))

    # Ensure YAML `lambda:` is wired up as the display writer.
    # (Some custom components rely on register_display, but depending on version
    # and schema usage this may not happen automatically.)
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(display.DisplayRef, "it")],
            return_type=cg.void,
        )
        cg.add(var.set_writer(lambda_))