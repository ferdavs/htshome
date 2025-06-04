import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["sensor"]
CODEOWNERS = ["@your-github-username"]

ble_advertiser_ns = cg.esphome_ns.namespace("ble_advertiser")
BLEAdvertiser = ble_advertiser_ns.class_("BLEAdvertiser", cg.Component)

CONF_TEMPERATURE = "temperature"
CONF_HUMIDITY = "humidity"
CONF_PRESSURE = "pressure"
CONF_CO2 = "co2"
CONF_PM1_0 = "pm1_0"
CONF_PM2_5 = "pm2_5"
CONF_PM10_0 = "pm10_0"
CONF_IAQ = "iaq"
CONF_BATTERY = "battery"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BLEAdvertiser),
    cv.Optional(CONF_TEMPERATURE): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_HUMIDITY): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_PRESSURE): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_CO2): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_PM1_0): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_PM2_5): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_PM10_0): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_IAQ): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_BATTERY): cv.use_id(sensor.Sensor),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_TEMPERATURE in config:
        temperature = await cg.get_variable(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature(temperature))

    if CONF_HUMIDITY in config:
        humidity = await cg.get_variable(config[CONF_HUMIDITY])
        cg.add(var.set_humidity(humidity))

    if CONF_PRESSURE in config:
        pressure = await cg.get_variable(config[CONF_PRESSURE])
        cg.add(var.set_pressure(pressure))

    if CONF_CO2 in config:
        co2 = await cg.get_variable(config[CONF_CO2])
        cg.add(var.set_co2(co2))

    if CONF_PM1_0 in config:
        pm1_0 = await cg.get_variable(config[CONF_PM1_0])
        cg.add(var.set_pm1_0(pm1_0))

    if CONF_PM2_5 in config:
        pm2_5 = await cg.get_variable(config[CONF_PM2_5])
        cg.add(var.set_pm2_5(pm2_5))

    if CONF_PM10_0 in config:
        pm10_0 = await cg.get_variable(config[CONF_PM10_0])
        cg.add(var.set_pm10_0(pm10_0))

    if CONF_IAQ in config:
        iaq = await cg.get_variable(config[CONF_IAQ])
        cg.add(var.set_iaq(iaq))

    if CONF_BATTERY in config:
        battery = await cg.get_variable(config[CONF_BATTERY])
        cg.add(var.set_battery(battery))
