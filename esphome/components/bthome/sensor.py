import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from . import (
    BTHome,
    CONF_BTHOME_ID,
    CONF_OID,
    oid_variable,
    unique_oid,
)

DEPENDENCIES = ["bthome"]

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        sensor.Sensor,
    ).extend({
        cv.GenerateID(CONF_BTHOME_ID): cv.use_id(BTHome),
        cv.Required(CONF_OID): cv.uint8_t,
    }).extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = unique_oid


async def to_code(config):
    parent = await cg.get_variable(config[CONF_BTHOME_ID])
    sens = await sensor.new_sensor(config)
    cg.add(parent.register_sensor(config[CONF_OID], oid_variable(config[CONF_OID]), sens))
