import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, remote_transmitter
from esphome.const import CONF_ID

samsung_ac_ns = cg.esphome_ns.namespace("samsung_ac_climate")
SamsungACClimate = samsung_ac_ns.class_("SamsungACClimate", climate.Climate, cg.Component)

# Correct RemoteTransmitter reference
RemoteTransmitter = remote_transmitter.remote_transmitter_ns.class_("RemoteTransmitter")

CONF_TURBO_SUPPORT = "turbo_support"
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_TRANSMITTER_PIN = "transmitter_pin"

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(SamsungACClimate),
    cv.Required(CONF_TRANSMITTER_ID): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
    cv.Required(CONF_TRANSMITTER_PIN): cv.uint8_t,
    cv.Optional(CONF_TURBO_SUPPORT, default=False): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))
    cg.add(var.set_transmitter_pin(config[CONF_TRANSMITTER_PIN]))
    cg.add(var.set_turbo_support(config[CONF_TURBO_SUPPORT]))
