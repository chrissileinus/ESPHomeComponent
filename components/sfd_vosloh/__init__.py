import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ['uart']

sfd_vosloh_ns = cg.esphome_ns.namespace('sfd_vosloh')
sfdVosloh = sfd_vosloh_ns.class_('sfdVosloh', cg.Component, uart.UARTDevice)

CONF_LINE_LENGTH = "line_length"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(sfdVosloh),
    cv.Optional(CONF_LINE_LENGTH, default=127): cv.int_range(min=1, max=127),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    cg.add(var.set_line_length(config[CONF_LINE_LENGTH]))
