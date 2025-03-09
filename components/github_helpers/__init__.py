from esphome.components import text_sensor, binary_sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import automation

DEPENDENCIES = ['network']

github_helpers_ns = cg.esphome_ns.namespace('github_helpers')
GitHubHelpers = github_helpers_ns.class_('GitHubHelpers', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(GitHubHelpers),
    cv.Optional('latest_commit_sensor'): text_sensor.TEXT_SENSOR_SCHEMA,
    cv.Optional('stored_commit_sensor'): text_sensor.TEXT_SENSOR_SCHEMA,
    cv.Optional('update_available_sensor'): binary_sensor.BINARY_SENSOR_SCHEMA,
    cv.Optional('update_message_sensor'): text_sensor.TEXT_SENSOR_SCHEMA,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[cv.GenerateID()])
    await cg.register_component(var, config)

    if 'latest_commit_sensor' in config:
        sens = await text_sensor.new_text_sensor(config['latest_commit_sensor'])
        cg.add(var.set_latest_commit_sensor(sens))

    if 'stored_commit_sensor' in config:
        sens = await text_sensor.new_text_sensor(config['stored_commit_sensor'])
        cg.add(var.set_stored_commit_sensor(sens))

    if 'update_available_sensor' in config:
        sens = await binary_sensor.new_binary_sensor(config['update_available_sensor'])
        cg.add(var.set_update_available_sensor(sens))

    if 'update_message_sensor' in config:
        sens = await text_sensor.new_text_sensor(config['update_message_sensor'])
        cg.add(var.set_update_message_sensor(sens))