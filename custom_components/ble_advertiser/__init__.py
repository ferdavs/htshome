import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

DEPENDENCIES = ['sensor']
CODEOWNERS = ['@h2zero']

ble_advertiser_ns = cg.esphome_ns.namespace('ble_advertiser')
BLEAdvertiser = ble_advertiser_ns.class_('BLEAdvertiser', cg.Component)

CONF_SENSORS = 'sensors'
CONF_LABEL = 'label'
CONF_UNIT = 'unit'
CONF_SCALE_FACTOR = 'scale_factor'
CONF_PRECISION = 'precision'
CONF_UPDATE_INTERVAL = 'update_interval'
CONF_MIN_INTERVAL = 'min_interval'
CONF_MAX_INTERVAL = 'max_interval'
CONF_DEVICE_NAME = 'device_name'
CONF_POWER_LEVEL = 'power_level'

SENSOR_SCHEMA = cv.Schema({
    cv.Required(CONF_ID): cv.use_id(sensor.Sensor),
    cv.Required(CONF_LABEL): cv.string,
    cv.Optional(CONF_UNIT, default=''): cv.string,
    cv.Optional(CONF_SCALE_FACTOR, default=1.0): cv.float_,
    cv.Optional(CONF_PRECISION, default=2): cv.int_range(min=0, max=10),
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BLEAdvertiser),
    cv.Required(CONF_SENSORS): cv.ensure_list(SENSOR_SCHEMA),
    cv.Optional(CONF_UPDATE_INTERVAL, default='10s'): cv.positive_time_period_seconds,
    cv.Optional(CONF_MIN_INTERVAL, default=32): cv.int_range(min=20, max=10240),
    cv.Optional(CONF_MAX_INTERVAL, default=64): cv.int_range(min=20, max=10240),
    cv.Optional(CONF_DEVICE_NAME, default='HTS-HOME'): cv.string,
    cv.Optional(CONF_POWER_LEVEL, default=9): cv.int_range(min=0, max=9),
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    cg.add(var.set_min_interval(config[CONF_MIN_INTERVAL]))
    cg.add(var.set_max_interval(config[CONF_MAX_INTERVAL]))
    cg.add(var.set_device_name(config[CONF_DEVICE_NAME]))
    cg.add(var.set_power_level(config[CONF_POWER_LEVEL]))
    
    for sensor_config in config[CONF_SENSORS]:
        sensor_var = yield cg.get_variable(sensor_config[CONF_ID])
        cg.add(var.add_sensor(
            sensor_var,
            sensor_config[CONF_LABEL],
            sensor_config[CONF_UNIT],
            sensor_config[CONF_SCALE_FACTOR],
            sensor_config[CONF_PRECISION]
        ))
