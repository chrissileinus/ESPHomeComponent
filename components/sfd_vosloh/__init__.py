import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, text_sensor, sensor, binary_sensor
from esphome.const import CONF_ID

AUTO_LOAD = ["text_sensor", "sensor", "binary_sensor"]
DEPENDENCIES = ['uart']

sfd_vosloh_ns = cg.esphome_ns.namespace('sfd_vosloh')
sfdVosloh = sfd_vosloh_ns.class_('sfdVosloh', cg.Component, uart.UARTDevice)

CONF_ROW_LENGTH = "row_length"
CONF_CURRENT_CONTENT = "current_content"
CONF_CURRENT_C_STATE = "current_c_state"
CONF_CURRENT_M_STATE = "current_m_state"
CONF_LAST_MODULE = "last_module"
CONF_ROLLING = "rolling"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(sfdVosloh),
    cv.Optional(CONF_ROW_LENGTH, default=127): cv.int_range(min=1, max=127),
    cv.Optional(CONF_CURRENT_CONTENT):
        text_sensor.TEXT_SENSOR_SCHEMA,
    cv.Optional(CONF_CURRENT_C_STATE):
        text_sensor.TEXT_SENSOR_SCHEMA,
    cv.Optional(CONF_CURRENT_M_STATE):
        text_sensor.TEXT_SENSOR_SCHEMA,
    cv.Optional(CONF_LAST_MODULE):
        sensor.SENSOR_SCHEMA,
    cv.Optional(CONF_ROLLING):
        binary_sensor.BINARY_SENSOR_SCHEMA,
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    cg.add(var.setup_row_length(config[CONF_ROW_LENGTH]))
    if CONF_CURRENT_CONTENT in config:
        conf = config[CONF_CURRENT_CONTENT]
        sens = yield text_sensor.new_text_sensor(conf)
        cg.add(var.setup_current_content(sens))
    if CONF_CURRENT_C_STATE in config:
        conf = config[CONF_CURRENT_C_STATE]
        sens = yield text_sensor.new_text_sensor(conf)
        cg.add(var.setup_current_c_state(sens))
    if CONF_CURRENT_M_STATE in config:
        conf = config[CONF_CURRENT_M_STATE]
        sens = yield text_sensor.new_text_sensor(conf)
        cg.add(var.setup_current_m_state(sens))
    if CONF_LAST_MODULE in config:
        conf = config[CONF_LAST_MODULE]
        sens = yield sensor.new_sensor(conf)
        cg.add(var.setup_last_module(sens))
    if CONF_ROLLING in config:
        conf = config[CONF_ROLLING]
        sens = yield binary_sensor.new_binary_sensor(conf)
        cg.add(var.setup_rolling(sens))
