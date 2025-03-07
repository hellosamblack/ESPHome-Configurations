import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@limengdu"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

sam_mr60bha2_ns = cg.esphome_ns.namespace("sam_seeed_mr60bha2")

SAM_MR60BHA2Component = sam_mr60bha2_ns.class_(
    "SAM_MR60BHA2Component", cg.Component, uart.UARTDevice
)

CONF_SAM_MR60BHA2_ID = "sam_mr60bha2_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SAM_MR60BHA2Component),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "sam_seeed_mr60bha2",
    require_tx=True,
    require_rx=True,
    baud_rate=115200,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)