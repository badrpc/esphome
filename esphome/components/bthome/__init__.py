import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker
from esphome.const import CONF_MAC_ADDRESS, CONF_ID


CODEOWNERS = ["@badrpc"]
DEPENDENCIES = ["esp32_ble_tracker"]
MULTI_CONF = True

CONF_ENCRYPTION_KEY = "encryption_key"
CONF_BTHOME_ID = "bthome_id"
CONF_OID = "oid"

bthome_ns = cg.esphome_ns.namespace("bthome")
BTHome = bthome_ns.class_("BTHome", esp32_ble_tracker.ESPBTDeviceListener, cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BTHome),
            cv.Optional(CONF_ENCRYPTION_KEY): cv.bind_key,
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIGURED_OIDS: dict[tuple[str, int], str] = {}


oid_range_validate = cv.int_range(min=1, max=255)


def unique_oid(config):
    bthome_id = config[CONF_BTHOME_ID]
    oid = config[CONF_OID]
    current_id = config[CONF_ID]
    prev_id = CONFIGURED_OIDS.get((bthome_id, oid), "")
    if prev_id != "":
        raise cv.Invalid(
            f"BTHome entities {prev_id} and {current_id} reuse the same OID {oid:#04x}"
        )
    CONFIGURED_OIDS[(bthome_id, oid)] = current_id


def oid_variable(oid: int):
    return cg.RawExpression(f"bthome::OID_{oid:02X}")


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_encryption_key(config[CONF_ENCRYPTION_KEY]))
