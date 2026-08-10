import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, remote_transmitter, sensor
from esphome.const import CONF_ID, CONF_SENSOR, UNIT_CELSIUS

samsung_ac_ns = cg.esphome_ns.namespace("samsung_ac_climate")
SamsungACClimate = samsung_ac_ns.class_("SamsungACClimate", climate.Climate, cg.Component)

RemoteTransmitter = remote_transmitter.remote_transmitter_ns.class_("RemoteTransmitter")

CONF_TURBO_SUPPORT = "turbo_support"
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_TRANSMITTER_PIN = "transmitter_pin"
CONF_PRESET_TIMEOUT = "preset_timeout"

CONFIG_SCHEMA = climate._CLIMATE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(SamsungACClimate),
    cv.Required(CONF_TRANSMITTER_ID): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
    cv.Required(CONF_TRANSMITTER_PIN): cv.uint8_t,
    cv.Optional(CONF_TURBO_SUPPORT, default=False): cv.boolean,
    cv.Optional(CONF_SENSOR): cv.use_id(sensor.Sensor),
    # How long BOOST/ECO stays active before auto-reverting to NONE and
    # restoring the remembered fan speed. Matches the unit's own Turbo
    # duration by default.
    cv.Optional(CONF_PRESET_TIMEOUT, default="30min"): cv.positive_time_period_milliseconds,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))
    cg.add(var.set_transmitter_pin(config[CONF_TRANSMITTER_PIN]))
    cg.add(var.set_turbo_support(config[CONF_TURBO_SUPPORT]))
    cg.add(var.set_preset_timeout(config[CONF_PRESET_TIMEOUT].total_milliseconds))

    if CONF_SENSOR in config:
        sensor_obj = await cg.get_variable(config[CONF_SENSOR])
        cg.add(var.set_temperature_sensor(sensor_obj))
