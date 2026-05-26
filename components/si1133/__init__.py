import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_ILLUMINANCE,
    STATE_CLASS_MEASUREMENT,
    UNIT_LUX,
)

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]

si1133_ns = cg.esphome_ns.namespace("si1133")
Si1133Component = si1133_ns.class_(
    "Si1133Component", cg.PollingComponent, i2c.I2CDevice
)

CONF_UV_INDEX      = "uv_index"
CONF_AMBIENT_LIGHT = "ambient_light"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Si1133Component),
            cv.Optional(CONF_UV_INDEX): sensor.sensor_schema(
                unit_of_measurement="UV index",
                accuracy_decimals=2,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_AMBIENT_LIGHT): sensor.sensor_schema(
                unit_of_measurement=UNIT_LUX,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x55))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_UV_INDEX in config:
        sens = await sensor.new_sensor(config[CONF_UV_INDEX])
        cg.add(var.set_uv_index_sensor(sens))

    if CONF_AMBIENT_LIGHT in config:
        sens = await sensor.new_sensor(config[CONF_AMBIENT_LIGHT])
        cg.add(var.set_ambient_light_sensor(sens))
