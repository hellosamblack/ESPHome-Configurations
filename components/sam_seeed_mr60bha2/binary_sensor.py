import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_OCCUPANCY,
    CONF_HAS_TARGET,
)
from . import CONF_SAM_MR60BHA2_ID, SAM_MR60BHA2Component

DEPENDENCIES = ["sam_seeed_mr60bha2"]

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_SAM_MR60BHA2_ID): cv.use_id(SAM_MR60BHA2Component),
    cv.Optional(CONF_HAS_TARGET): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_OCCUPANCY, icon="mdi:motion-sensor"
    ),
}


async def to_code(config):
    sam_mr60bha2_component = await cg.get_variable(config[CONF_SAM_MR60BHA2_ID])

    if has_target_config := config.get(CONF_HAS_TARGET):
        sens = await binary_sensor.new_binary_sensor(has_target_config)
        cg.add(sam_mr60bha2_component.set_has_target_binary_sensor(sens))
