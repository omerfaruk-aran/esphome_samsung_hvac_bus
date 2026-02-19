import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import (
    uart,
    sensor,
    switch,
    select,
    number,
    climate,
    text_sensor,
)
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_WATT,
    UNIT_VOLT,
    UNIT_AMPERE,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_DEVICE_CLASS,
    CONF_FILTERS,
    CONF_ICON,
    CONF_ACCURACY_DECIMALS,
    CONF_ENTITY_CATEGORY,
    CONF_STATE_CLASS,
)
from esphome.core import CORE, Lambda

CODEOWNERS = ["matthias882", "lanwin", "omerfaruk-aran"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "switch", "select", "number", "climate"]
MULTI_CONF = False

CONF_SAMSUNG_AC_ID = "samsung_ac_id"

samsung_ac = cg.esphome_ns.namespace("samsung_ac")
Samsung_AC = samsung_ac.class_("Samsung_AC", cg.PollingComponent, uart.UARTDevice)
Samsung_AC_Device = samsung_ac.class_("Samsung_AC_Device")
Samsung_AC_Switch = samsung_ac.class_("Samsung_AC_Switch", switch.Switch)
Samsung_AC_Mode_Select = samsung_ac.class_("Samsung_AC_Mode_Select", select.Select)
Samsung_AC_Water_Heater_Mode_Select = samsung_ac.class_(
    "Samsung_AC_Water_Heater_Mode_Select", select.Select
)
Samsung_AC_Number = samsung_ac.class_("Samsung_AC_Number", number.Number)
Samsung_AC_Climate = samsung_ac.class_("Samsung_AC_Climate", climate.Climate)

# not sure why select.select_schema did not work yet
SELECT_MODE_SCHEMA = select.select_schema(Samsung_AC_Mode_Select)
SELECT_WATER_HEATER_MODE_SCHEMA = select.select_schema(
    Samsung_AC_Water_Heater_Mode_Select
)

NUMBER_SCHEMA = number.number_schema(Samsung_AC_Number).extend(
    {cv.GenerateID(): cv.declare_id(Samsung_AC_Number)}
)

CLIMATE_SCHEMA = climate.climate_schema(Samsung_AC_Climate).extend(
    {cv.GenerateID(): cv.declare_id(Samsung_AC_Climate)}
)

CONF_DEVICE_ID = "samsung_ac_device_id"
CONF_DEVICE_ADDRESS = "address"
CONF_DEVICE_ROOM_TEMPERATURE = "room_temperature"
CONF_DEVICE_ROOM_TEMPERATURE_OFFSET = "room_temperature_offset"
CONF_DEVICE_TARGET_TEMPERATURE = "target_temperature"
CONF_DEVICE_WATER_OUTLET_TARGET = "water_outlet_target"
CONF_DEVICE_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_DEVICE_INDOOR_EVA_IN_TEMPERATURE = "indoor_eva_in_temperature"
CONF_DEVICE_INDOOR_EVA_OUT_TEMPERATURE = "indoor_eva_out_temperature"
CONF_DEVICE_WATER_TEMPERATURE = "water_temperature"
CONF_DEVICE_WATER_TARGET_TEMPERATURE = "water_target_temperature"
CONF_DEVICE_POWER = "power"
CONF_DEVICE_AUTOMATIC_CLEANING = "automatic_cleaning"
CONF_DEVICE_WATER_HEATER_POWER = "water_heater_power"
CONF_DEVICE_MODE = "mode"
CONF_DEVICE_WATER_HEATER_MODE = "water_heater_mode"
CONF_DEVICE_CLIMATE = "climate"
CONF_DEVICE_ROOM_HUMIDITY = "room_humidity"
CONF_DEVICE_CUSTOM = "custom_sensor"
CONF_DEVICE_CUSTOM_MESSAGE = "message"
CONF_DEVICE_CUSTOM_RAW_FILTERS = "raw_filters"
CONF_DEVICE_ERROR_CODE = "error_code"
CONF_DEVICE_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM = "outdoor_instantaneous_power"
CONF_DEVICE_OUT_CONTROL_WATTMETER_1W_1MIN_SUM = "outdoor_cumulative_energy"
CONF_DEVICE_OUT_SENSOR_CT1 = "outdoor_current"
CONF_DEVICE_OUT_SENSOR_VOLTAGE = "outdoor_voltage"

CONF_DEVICE_PIPE_IN3_TEMPERATURE = "pipe_in3_temperature"
CONF_DEVICE_PIPE_IN4_TEMPERATURE = "pipe_in4_temperature"
CONF_DEVICE_PIPE_IN5_TEMPERATURE = "pipe_in5_temperature"
CONF_DEVICE_PIPE_OUT1_TEMPERATURE = "pipe_out1_temperature"
CONF_DEVICE_WATTMETER_1UNIT = "wattmeter_1unit"
CONF_DEVICE_WATTMETER_TOTAL_SUM = "wattmeter_total_sum"
CONF_DEVICE_WATTMETER_TOTAL_SUM_ACCUM = "wattmeter_total_sum_accum"
CONF_DEVICE_WATTMETER_TOTAL_PRODUCED = "wattmeter_total_produced"
CONF_DEVICE_WATTMETER_ACTUAL_PRODUCED = "wattmeter_actual_produced"
CONF_DEVICE_CAPACITY_REQUEST = "capacity_request"

CONF_CAPABILITIES = "capabilities"
CONF_CAPABILITIES_HORIZONTAL_SWING = "horizontal_swing"
CONF_CAPABILITIES_VERTICAL_SWING = "vertical_swing"

CONF_PRESETS = "presets"
CONF_PRESET_NAME = "name"
CONF_PRESET_ENABLED = "enabled"
CONF_PRESET_VALUE = "value"

# Add these at the top with the other CONF_DEVICE_* constants
CONF_DEVICE_COMPRESSOR_TARGET_FREQUENCY = "compressor_target_frequency"
CONF_DEVICE_FAN_POWER = "fan_power"
CONF_DEVICE_TOTAL_POWER = "total_power"
CONF_DEVICE_COMPRESSOR_COMMANDED_FREQUENCY = "compressor_commanded_frequency"
CONF_DEVICE_PHASE_CURRENT = "phase_current"
CONF_DEVICE_PHASE_VOLTAGE = "phase_voltage"
CONF_DEVICE_PHASE_POWER = "phase_power"
CONF_DEVICE_POWER_FACTOR = "power_factor"
CONF_DEVICE_SYSTEM_EFFICIENCY = "system_efficiency"
CONF_DEVICE_SYSTEM_CAPACITY = "system_capacity"
CONF_DEVICE_TOTAL_ENERGY_CONSUMPTION = "total_energy_consumption"
CONF_DEVICE_OPERATION_HOURS = "operation_hours"
CONF_DEVICE_COMPRESSOR_HOURS = "compressor_hours"
CONF_DEVICE_FAN_HOURS = "fan_hours"
CONF_DEVICE_CONTROL_WATTMETER_UNIT = "control_wattmeter_unit"
CONF_DEVICE_WATTMETER_ACCUMULATED = "wattmeter_accumulated"
CONF_DEVICE_PRODUCED_ENERGY_ACTUAL = "produced_energy_actual"
CONF_DEVICE_PRODUCED_ENERGY_TOTAL = "produced_energy_total"
CONF_DEVICE_COMPRESSOR_CURRENT_FREQ = "compressor_current_freq"
CONF_DEVICE_OUTDOOR_OPERATION_MODE = "outdoor_operation_mode" 
CONF_DEVICE_OPERATION_MODE = "operation_mode" 

# Add new constants for indoor custom sensors
CONF_DEVICE_OUTDOOR_AIR_SENSOR = "outdoor_air_sensor"
CONF_DEVICE_DEFROST_MODE = "defrost_mode"
CONF_DEVICE_WATER_OUTLET_TARGET_TEMP = "water_outlet_target_temperature"
CONF_DEVICE_DHW_OPERATION_ENABLE = "dhw_operation_enable"
CONF_DEVICE_DHW_MODE_SETTING = "dhw_mode_setting"
CONF_DEVICE_DHW_TARGET_TEMP = "dhw_target_temperature"
CONF_DEVICE_BACKUP_HEATER_STATUS = "backup_heater_status"
CONF_DEVICE_BOOSTER_HEATER_STATUS = "booster_heater_status"
CONF_DEVICE_THREE_WAY_VALVE_POSITION = "three_way_valve_position"
CONF_DEVICE_CIRCULATION_PUMP_SPEED = "circulation_pump_speed"
CONF_DEVICE_WATER_FLOW_RATE = "water_flow_rate"
CONF_DEVICE_WATER_PRESSURE = "water_pressure"
CONF_DEVICE_HEAT_PUMP_OPERATION_MODE = "heat_pump_operation_mode"
CONF_DEVICE_DHW_TANK_TEMPERATURE = "dhw_tank_temperature"
CONF_DEVICE_WATER_HEATER_TEMPERATURE = "water_heater_temperature"

# Add new constants for missing outdoor unit sensors
CONF_DEVICE_OPERATION_MODE_OUTDOOR = "operation_mode_outdoor"
CONF_DEVICE_COMPRESSOR_RUNNING_STATUS = "compressor_running_status"
CONF_DEVICE_HOT_GAS_VALVE_STATUS = "hot_gas_valve_status"
CONF_DEVICE_LIQUID_LINE_VALVE_STATUS = "liquid_line_valve_status"
CONF_DEVICE_FOUR_WAY_REVERSING_VALVE_STATUS = "four_way_reversing_valve_status"
CONF_DEVICE_EVI_BYPASS_VALVE_STATUS = "evi_bypass_valve_status"
CONF_DEVICE_BASE_PAN_HEATER = "base_pan_heater"
CONF_DEVICE_PHE_HEATER = "phe_heater"
CONF_DEVICE_HIGH_PRESSURE = "high_pressure"
CONF_DEVICE_LOW_PRESSURE = "low_pressure"
CONF_DEVICE_DISCHARGE_TEMPERATURE = "discharge_temperature"
CONF_DEVICE_CONDENSER_OUTLET_TEMP = "condenser_outlet_temp"
CONF_DEVICE_SUCTION_TEMPERATURE = "suction_temperature"
CONF_DEVICE_EVI_INLET_TEMP = "evi_inlet_temp"
CONF_DEVICE_EVI_OUTLET_TEMP = "evi_outlet_temp"
CONF_DEVICE_OUTDOOR_FAN_STEP = "outdoor_fan_step"
CONF_DEVICE_MAIN_EEV_STEPS = "main_eev_steps"
CONF_DEVICE_DC_LINK_VOLTAGE = "dc_link_voltage"
CONF_DEVICE_HIGH_PRESSURE_SAT_TEMP = "high_pressure_sat_temp"
CONF_DEVICE_LOW_PRESSURE_SAT_TEMP = "low_pressure_sat_temp"
CONF_DEVICE_WATTMETER_TOTAL_SUM = "wattmeter_total_sum"
CONF_DEVICE_PRODUCED_ENERGY_TOTAL_NEW = "produced_energy_total_new"

# Add new constants for missing indoor unit sensors
CONF_DEVICE_ZONE2_ROOM_SETPOINT = "zone2_room_setpoint"
CONF_DEVICE_ZONE2_WATER_SETPOINT = "zone2_water_setpoint"
CONF_DEVICE_WATER_OUTLET_ZONE1 = "water_outlet_zone1"
CONF_DEVICE_WATER_OUTLET_ZONE2 = "water_outlet_zone2"
CONF_DEVICE_FLOW_RATE_CONTROL = "flow_rate_control"

# Add corrected constants
CONF_DEVICE_COMPRESSOR_POWER = "compressor_power"
CONF_DEVICE_FAN_COMPRESSOR_FREQUENCY = "fan_compressor_frequency"


def preset_entry(name: str, value: int, displayName: str):
    return (
        cv.Optional(name, default=False),
        cv.Any(
            cv.boolean,
            cv.All(
                {
                    cv.Optional(CONF_PRESET_ENABLED, default=False): cv.boolean,
                    cv.Optional(CONF_PRESET_NAME, default=displayName): cv.string,
                    cv.Optional(CONF_PRESET_VALUE, default=value): cv.int_,
                }
            ),
        ),
    )


PRESETS = {
    "sleep": {"value": 1, "displayName": "Sleep"},
    "quiet": {"value": 2, "displayName": "Quiet"},
    "fast": {"value": 3, "displayName": "Fast"},
    "longreach": {"value": 6, "displayName": "LongReach"},
    "eco": {"value": 7, "displayName": "Eco"},
    "windfree": {"value": 9, "displayName": "WindFree"},
}

CAPABILITIES_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_CAPABILITIES_HORIZONTAL_SWING, default=False): cv.boolean,
        cv.Optional(CONF_CAPABILITIES_VERTICAL_SWING, default=False): cv.boolean,
        cv.Optional(CONF_PRESETS): cv.Schema(
            dict(
                [
                    preset_entry(
                        name, PRESETS[name]["value"], PRESETS[name]["displayName"]
                    )
                    for name in PRESETS
                ]
            )
        ),
    }
)

CUSTOM_SENSOR_SCHEMA = sensor.sensor_schema().extend(
    {
        cv.Required(CONF_DEVICE_CUSTOM_MESSAGE): cv.hex_int,
    }
)


def custom_sensor_schema(
    message: int,
    unit_of_measurement=cv.UNDEFINED,
    icon=cv.UNDEFINED,
    accuracy_decimals=cv.UNDEFINED,
    device_class=cv.UNDEFINED,
    state_class=cv.UNDEFINED,
    entity_category=cv.UNDEFINED,
    raw_filters=[],
):
    schema = sensor.sensor_schema(
        unit_of_measurement=unit_of_measurement,
        icon=icon,
        accuracy_decimals=accuracy_decimals,
        device_class=device_class,
        state_class=state_class,
        entity_category=entity_category,
    ).extend(
        {
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=message): cv.hex_int,
            cv.Optional(
                CONF_DEVICE_CUSTOM_RAW_FILTERS, default=raw_filters
            ): sensor.validate_filters,
        }
    )

    return schema

def temperature_sensor_schema(message: int):
    return custom_sensor_schema(
        message=message,
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        raw_filters=[{"lambda": Lambda("return (int16_t)x;")}, {"multiply": 0.1}],
    )


def humidity_sensor_schema(message: int):
    return custom_sensor_schema(
        message=message,
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_HUMIDITY,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def error_code_sensor_schema(message: int):
    return custom_sensor_schema(
        message=message,
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=0,
        icon="mdi:alert",
        entity_category="diagnostic",
    )


DEVICE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DEVICE_ID): cv.declare_id(Samsung_AC_Device),
        cv.Optional(CONF_CAPABILITIES): CAPABILITIES_SCHEMA,
        cv.Required(CONF_DEVICE_ADDRESS): cv.string,
        cv.Optional(CONF_DEVICE_ROOM_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DEVICE_ROOM_TEMPERATURE_OFFSET): cv.float_,
        cv.Optional(CONF_DEVICE_OUTDOOR_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DEVICE_INDOOR_EVA_IN_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DEVICE_INDOOR_EVA_OUT_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DEVICE_ERROR_CODE): error_code_sensor_schema(0x8235),
        cv.Optional(CONF_DEVICE_TARGET_TEMPERATURE): NUMBER_SCHEMA,
        cv.Optional(CONF_DEVICE_WATER_OUTLET_TARGET): NUMBER_SCHEMA,
        cv.Optional(CONF_DEVICE_WATER_TARGET_TEMPERATURE): NUMBER_SCHEMA,
        cv.Optional(CONF_DEVICE_POWER): switch.switch_schema(
            Samsung_AC_Switch, icon="mdi:power"
        ),
        cv.Optional(CONF_DEVICE_AUTOMATIC_CLEANING): switch.switch_schema(
            Samsung_AC_Switch, icon="mdi:broom"
        ),
        cv.Optional(CONF_DEVICE_WATER_HEATER_POWER): switch.switch_schema(
            Samsung_AC_Switch
        ),
        cv.Optional(CONF_DEVICE_MODE): SELECT_MODE_SCHEMA,
        cv.Optional(CONF_DEVICE_WATER_HEATER_MODE): SELECT_WATER_HEATER_MODE_SCHEMA,
        cv.Optional(CONF_DEVICE_CLIMATE): CLIMATE_SCHEMA,
        cv.Optional(CONF_DEVICE_CUSTOM, default=[]): cv.ensure_list(
            CUSTOM_SENSOR_SCHEMA
        ),
        # keep CUSTOM_SENSOR_KEYS in sync with these
        cv.Optional(CONF_DEVICE_WATER_TEMPERATURE): temperature_sensor_schema(0x4237),
        cv.Optional(CONF_DEVICE_ROOM_HUMIDITY): humidity_sensor_schema(0x4038),
        cv.Optional(
            CONF_DEVICE_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM
        ): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ),
        cv.Optional(
            CONF_DEVICE_OUT_CONTROL_WATTMETER_1W_1MIN_SUM
        ): sensor.sensor_schema(
            unit_of_measurement="kWh",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ).extend(
            {
                cv.Optional(
                    CONF_FILTERS, default=[{"multiply": 0.001}]
                ): sensor.validate_filters
            }
        ),
        cv.Optional(CONF_DEVICE_OUT_SENSOR_CT1): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-ac",
        ).extend(
            {
                cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8217): cv.hex_int,
                cv.Optional(
                    CONF_FILTERS, default=[{"multiply": 0.1}]
                ): sensor.validate_filters,
            }
        ),
        cv.Optional(CONF_DEVICE_OUT_SENSOR_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ).extend(
            {
                cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x24FC): cv.hex_int,
            }
        ),
        # Add new temperature sensors
        cv.Optional(CONF_DEVICE_PIPE_IN3_TEMPERATURE): temperature_sensor_schema(0x4238),
        cv.Optional(CONF_DEVICE_PIPE_IN4_TEMPERATURE): temperature_sensor_schema(0x4239),
        cv.Optional(CONF_DEVICE_PIPE_IN5_TEMPERATURE): temperature_sensor_schema(0x423A),
        cv.Optional(CONF_DEVICE_PIPE_OUT1_TEMPERATURE): temperature_sensor_schema(0x423B),
        # Add new energy monitoring sensors
        cv.Optional(CONF_DEVICE_WATTMETER_1UNIT): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ),
        cv.Optional(CONF_DEVICE_WATTMETER_TOTAL_SUM): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ),
        cv.Optional(CONF_DEVICE_WATTMETER_TOTAL_SUM_ACCUM): sensor.sensor_schema(
            unit_of_measurement="kWh",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ),
        cv.Optional(CONF_DEVICE_WATTMETER_TOTAL_PRODUCED): sensor.sensor_schema(
            unit_of_measurement="kWh",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ),
        cv.Optional(CONF_DEVICE_WATTMETER_ACTUAL_PRODUCED): sensor.sensor_schema(
            unit_of_measurement="kWh",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ),
        cv.Optional(CONF_DEVICE_CAPACITY_REQUEST): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            icon="mdi:gauge",
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DEVICE_COMPRESSOR_TARGET_FREQUENCY): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8237): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_FAN_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:fan",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8239): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_TOTAL_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x823d): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_COMPRESSOR_COMMANDED_FREQUENCY): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-ac",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8236): cv.hex_int,
            cv.Optional(CONF_FILTERS, default=[{"multiply": 0.1}]): sensor.validate_filters,
        }),
        cv.Optional(CONF_DEVICE_COMPRESSOR_CURRENT_FREQ): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-ac",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8238): cv.hex_int,
            cv.Optional(CONF_FILTERS, default=[{"multiply": 0.1}]): sensor.validate_filters,
        }),
        cv.Optional(CONF_DEVICE_PHASE_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-ac",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x82db): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_PHASE_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x82de): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_PHASE_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x82df): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_POWER_FACTOR): sensor.sensor_schema(
            unit_of_measurement="",
            accuracy_decimals=2,
            icon="mdi:alpha-f-circle",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x82e8): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_SYSTEM_EFFICIENCY): sensor.sensor_schema(
            unit_of_measurement="",
            accuracy_decimals=1,
            icon="mdi:percent",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x82ed): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_SYSTEM_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="kW",
            accuracy_decimals=1,
            icon="mdi:gauge",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x82f6): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_TOTAL_ENERGY_CONSUMPTION): sensor.sensor_schema(
            unit_of_measurement="kWh",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8408): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_OPERATION_HOURS): sensor.sensor_schema(
            unit_of_measurement="h",
            accuracy_decimals=1,
            icon="mdi:clock-outline",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x840b): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_COMPRESSOR_HOURS): sensor.sensor_schema(
            unit_of_measurement="h",
            accuracy_decimals=1,
            icon="mdi:clock-outline",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x840c): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_FAN_HOURS): sensor.sensor_schema(
            unit_of_measurement="h",
            accuracy_decimals=1,
            icon="mdi:clock-outline",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x841a): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_CONTROL_WATTMETER_UNIT): sensor.sensor_schema(
            unit_of_measurement="kWh",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8411): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_WATTMETER_ACCUMULATED): sensor.sensor_schema(
            unit_of_measurement="kWh",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8416): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_OPERATION_MODE): sensor.sensor_schema(
            unit_of_measurement="",
            accuracy_decimals=0,
            icon="mdi:state-machine",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x4001): cv.hex_int,
        }),
        cv.Optional(CONF_DEVICE_OUTDOOR_OPERATION_MODE): sensor.sensor_schema(
            unit_of_measurement="",
            accuracy_decimals=0,
            icon="mdi:state-machine",
        ).extend({
            cv.Optional(CONF_DEVICE_CUSTOM_MESSAGE, default=0x8001): cv.hex_int,
        }),
        cv.Optional(
            CONF_DEVICE_OUT_OPERATION_ODU_MODE_TEXT
        ): text_sensor.text_sensor_schema(
            icon="mdi:fan",
            entity_category="diagnostic",
        ),
        cv.Optional(
            CONF_DEVICE_OUT_OPERATION_HEATCOOL_TEXT
        ): text_sensor.text_sensor_schema(
            icon="mdi:thermometer",
            entity_category="diagnostic",
        ),
    }
)

CUSTOM_SENSOR_KEYS = [
    # Outdoor custom sensors
    CONF_DEVICE_OUTDOOR_AIR_SENSOR,  # 0x8204
    CONF_DEVICE_DEFROST_MODE,        # 0x8031
    
    # Indoor custom sensors
    CONF_DEVICE_WATER_OUTLET_TARGET_TEMP,  # 0x4247
    CONF_DEVICE_DHW_OPERATION_ENABLE,      # 0x4065
    CONF_DEVICE_DHW_MODE_SETTING,          # 0x4066
    CONF_DEVICE_DHW_TARGET_TEMP,           # 0x4235
    CONF_DEVICE_BACKUP_HEATER_STATUS,      # 0x406C
    CONF_DEVICE_BOOSTER_HEATER_STATUS,     # 0x4087
    CONF_DEVICE_THREE_WAY_VALVE_POSITION,  # 0x4067
    CONF_DEVICE_CIRCULATION_PUMP_SPEED,    # 0x40C4
    CONF_DEVICE_WATER_FLOW_RATE,           # 0x4238
    CONF_DEVICE_WATER_PRESSURE,            # 0x4239
    CONF_DEVICE_HEAT_PUMP_OPERATION_MODE,  # 0x8001
    CONF_DEVICE_DHW_TANK_TEMPERATURE,      # 0x4237
    CONF_DEVICE_WATER_HEATER_TEMPERATURE,  # 0x42E9
    CONF_DEVICE_PRODUCED_ENERGY_ACTUAL,    # 0x8426
    CONF_DEVICE_PRODUCED_ENERGY_TOTAL,     # 0x8404
    
    # New outdoor unit sensors from reference tables
    CONF_DEVICE_OPERATION_MODE_OUTDOOR,    # 0x8001
    CONF_DEVICE_COMPRESSOR_RUNNING_STATUS, # 0x8010
    CONF_DEVICE_HOT_GAS_VALVE_STATUS,      # 0x8017
    CONF_DEVICE_LIQUID_LINE_VALVE_STATUS,  # 0x8019
    CONF_DEVICE_FOUR_WAY_REVERSING_VALVE_STATUS, # 0x801A
    CONF_DEVICE_EVI_BYPASS_VALVE_STATUS,   # 0x8021
    CONF_DEVICE_BASE_PAN_HEATER,           # 0x80AF
    CONF_DEVICE_PHE_HEATER,                # 0x80D7
    CONF_DEVICE_HIGH_PRESSURE,             # 0x8206
    CONF_DEVICE_LOW_PRESSURE,              # 0x8208
    CONF_DEVICE_DISCHARGE_TEMPERATURE,     # 0x820A
    CONF_DEVICE_CONDENSER_OUTLET_TEMP,     # 0x8218
    CONF_DEVICE_SUCTION_TEMPERATURE,       # 0x821A
    CONF_DEVICE_EVI_INLET_TEMP,            # 0x821E
    CONF_DEVICE_EVI_OUTLET_TEMP,           # 0x8220
    CONF_DEVICE_OUTDOOR_FAN_STEP,          # 0x8226
    CONF_DEVICE_MAIN_EEV_STEPS,            # 0x8229
    CONF_DEVICE_DC_LINK_VOLTAGE,           # 0x823B
    CONF_DEVICE_HIGH_PRESSURE_SAT_TEMP,    # 0x829F
    CONF_DEVICE_LOW_PRESSURE_SAT_TEMP,     # 0x82A0
    CONF_DEVICE_WATTMETER_TOTAL_SUM,       # 0x8415
    CONF_DEVICE_PRODUCED_ENERGY_TOTAL_NEW, # 0x8427
    
    # New indoor unit sensors from reference tables
    CONF_DEVICE_ZONE2_ROOM_SETPOINT,       # 0x42D6
    CONF_DEVICE_ZONE2_WATER_SETPOINT,      # 0x42D7
    CONF_DEVICE_WATER_OUTLET_ZONE1,        # 0x42D8
    CONF_DEVICE_WATER_OUTLET_ZONE2,        # 0x42D9
    CONF_DEVICE_FLOW_RATE_CONTROL,         # 0x42F1
    
    # Corrected constants
    CONF_DEVICE_COMPRESSOR_POWER,          # 0x8237
    CONF_DEVICE_FAN_COMPRESSOR_FREQUENCY,  # 0x8238
]

CONF_DEVICES = "devices"

CONF_DEBUG_MQTT_HOST = "debug_mqtt_host"
CONF_DEBUG_MQTT_PORT = "debug_mqtt_port"
CONF_DEBUG_MQTT_USERNAME = "debug_mqtt_username"
CONF_DEBUG_MQTT_PASSWORD = "debug_mqtt_password"

CONF_DEBUG_LOG_MESSAGES = "debug_log_messages"
CONF_DEBUG_LOG_MESSAGES_RAW = "debug_log_messages_raw"

CONF_NON_NASA_KEEPALIVE = "non_nasa_keepalive"

CONF_DEBUG_LOG_UNDEFINED_MESSAGES = "debug_log_undefined_messages"


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Samsung_AC),
            # cv.Optional(CONF_PAUSE, default=False): cv.boolean,
            cv.Optional(CONF_DEBUG_MQTT_HOST, default=""): cv.string,
            cv.Optional(CONF_DEBUG_MQTT_PORT, default=1883): cv.int_,
            cv.Optional(CONF_DEBUG_MQTT_USERNAME, default=""): cv.string,
            cv.Optional(CONF_DEBUG_MQTT_PASSWORD, default=""): cv.string,
            cv.Optional(CONF_DEBUG_LOG_MESSAGES, default=False): cv.boolean,
            cv.Optional(CONF_DEBUG_LOG_MESSAGES_RAW, default=False): cv.boolean,
            cv.Optional(CONF_NON_NASA_KEEPALIVE, default=False): cv.boolean,
            cv.Optional(CONF_NON_NASA_TX_DELAY_MS, default=0): cv.int_range(
                min=0, max=1000
            ),
            cv.Optional(CONF_DEBUG_LOG_UNDEFINED_MESSAGES, default=False): cv.boolean,
            cv.Optional(CONF_CAPABILITIES): CAPABILITIES_SCHEMA,
            cv.Required(CONF_DEVICES): cv.ensure_list(DEVICE_SCHEMA),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("30s"))
)


async def to_code(config):
    # For Debug_MQTT
    if CORE.is_esp8266 or CORE.is_libretiny:
        cg.add_library("heman/AsyncMqttClient-esphome", "2.0.0")

    var = cg.new_Pvariable(config[CONF_ID])
    for device_index, device in enumerate(config[CONF_DEVICES]):
        var_dev = cg.new_Pvariable(
            device[CONF_DEVICE_ID], device[CONF_DEVICE_ADDRESS], var
        )

        # setup capabilities
        capabilities = device.get(CONF_CAPABILITIES, config.get(CONF_CAPABILITIES, {}))

        cg.add(
            var_dev.set_supports_fan_modes(
                capabilities.get(CONF_CAPABILITIES_FAN_MODES, True)
            )
        )


        if CONF_CAPABILITIES_VERTICAL_SWING in capabilities:
            cg.add(
                var_dev.set_supports_vertical_swing(
                    capabilities[CONF_CAPABILITIES_VERTICAL_SWING]
                )
            )

        if CONF_CAPABILITIES_HORIZONTAL_SWING in capabilities:
            cg.add(
                var_dev.set_supports_horizontal_swing(
                    capabilities[CONF_CAPABILITIES_HORIZONTAL_SWING]
                )
            )

        none_added = False
        presets = capabilities.get(CONF_PRESETS, {})

        for preset, preset_info in PRESETS.items():
            preset_conf = presets.get(preset, None)

            if isinstance(preset_conf, bool) and preset_conf:
                if not none_added:
                    none_added = True
                    cg.add(var_dev.add_alt_mode("None", 0))

                cg.add(
                    var_dev.add_alt_mode(
                        preset_info["displayName"], preset_info["value"]
                    )
                )
            elif isinstance(preset_conf, dict) and preset_conf.get(
                CONF_PRESET_ENABLED, False
            ):
                if not none_added:
                    none_added = True
                    cg.add(var_dev.add_alt_mode("None", 0))

                cg.add(
                    var_dev.add_alt_mode(
                        preset_conf.get(CONF_PRESET_NAME, preset_info["displayName"]),
                        preset_conf.get(CONF_PRESET_VALUE, preset_info["value"]),
                    )
                )

        #        if CONF_CAPABILITIES in device and CONF_ALT_MODES in device[CONF_CAPABILITIES]:
        #            cg.add(var_dev.add_alt_mode("None", 0))
        #            for alt in device[CONF_CAPABILITIES][CONF_ALT_MODES]:
        #                cg.add(var_dev.add_alt_mode(alt[CONF_ALT_MODE_NAME], alt[CONF_ALT_MODE_VALUE]))
        #        elif CONF_CAPABILITIES in config and CONF_ALT_MODES in config[CONF_CAPABILITIES]:
        #            cg.add(var_dev.add_alt_mode("None", 0))
        #            for alt in config[CONF_CAPABILITIES][CONF_ALT_MODES]:
        #                cg.add(var_dev.add_alt_mode(alt[CONF_ALT_MODE_NAME], alt[CONF_ALT_MODE_VALUE]))

        # Mapping of config keys to their corresponding methods and types
        device_actions = {
            CONF_DEVICE_POWER: (switch.new_switch, var_dev.set_power_switch),
            CONF_DEVICE_AUTOMATIC_CLEANING: (
                switch.new_switch,
                var_dev.set_automatic_cleaning_switch,
            ),
            CONF_DEVICE_WATER_HEATER_POWER: (
                switch.new_switch,
                var_dev.set_water_heater_power_switch,
            ),
            CONF_DEVICE_ROOM_TEMPERATURE: (
                sensor.new_sensor,
                var_dev.set_room_temperature_sensor,
            ),
            CONF_DEVICE_OUTDOOR_TEMPERATURE: (
                sensor.new_sensor,
                var_dev.set_outdoor_temperature_sensor,
            ),
            CONF_DEVICE_INDOOR_EVA_IN_TEMPERATURE: (
                sensor.new_sensor,
                var_dev.set_indoor_eva_in_temperature_sensor,
            ),
            CONF_DEVICE_INDOOR_EVA_OUT_TEMPERATURE: (
                sensor.new_sensor,
                var_dev.set_indoor_eva_out_temperature_sensor,
            ),
            CONF_DEVICE_ERROR_CODE: (sensor.new_sensor, var_dev.set_error_code_sensor),
            CONF_DEVICE_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM: (
                sensor.new_sensor,
                var_dev.set_outdoor_instantaneous_power_sensor,
            ),
            CONF_DEVICE_OUT_CONTROL_WATTMETER_1W_1MIN_SUM: (
                sensor.new_sensor,
                var_dev.set_outdoor_cumulative_energy_sensor,
            ),
            CONF_DEVICE_OUT_SENSOR_CT1: (
                sensor.new_sensor,
                var_dev.set_outdoor_current_sensor,
            ),
            CONF_DEVICE_OUT_SENSOR_VOLTAGE: (
                sensor.new_sensor,
                var_dev.set_outdoor_voltage_sensor,
            ),
            CONF_DEVICE_PIPE_IN3_TEMPERATURE: (sensor.new_sensor, var_dev.set_pipe_in3_temperature_sensor),
            CONF_DEVICE_PIPE_IN4_TEMPERATURE: (sensor.new_sensor, var_dev.set_pipe_in4_temperature_sensor),
            CONF_DEVICE_PIPE_IN5_TEMPERATURE: (sensor.new_sensor, var_dev.set_pipe_in5_temperature_sensor),
            CONF_DEVICE_PIPE_OUT1_TEMPERATURE: (sensor.new_sensor, var_dev.set_pipe_out1_temperature_sensor),
            CONF_DEVICE_WATTMETER_1UNIT: (sensor.new_sensor, var_dev.set_wattmeter_1unit_sensor),
            CONF_DEVICE_WATTMETER_TOTAL_SUM: (sensor.new_sensor, var_dev.set_wattmeter_total_sum_sensor),
            CONF_DEVICE_WATTMETER_TOTAL_SUM_ACCUM: (sensor.new_sensor, var_dev.set_wattmeter_total_sum_accum_sensor),
            CONF_DEVICE_WATTMETER_TOTAL_PRODUCED: (sensor.new_sensor, var_dev.set_wattmeter_total_produced_sensor),
            CONF_DEVICE_WATTMETER_ACTUAL_PRODUCED: (sensor.new_sensor, var_dev.set_wattmeter_actual_produced_sensor),
            CONF_DEVICE_CAPACITY_REQUEST: (sensor.new_sensor, var_dev.set_capacity_request_sensor),
            CONF_DEVICE_OUT_OPERATION_ODU_MODE_TEXT: (
                text_sensor.new_text_sensor,
                var_dev.set_outdoor_operation_odu_mode_text_sensor,
            ),
            CONF_DEVICE_OUT_OPERATION_HEATCOOL_TEXT: (
                text_sensor.new_text_sensor,
                var_dev.set_outdoor_operation_heatcool_text_sensor,
            ),
        }

        # Add new protocol message sensors to device_actions
        device_actions.update({
            CONF_DEVICE_COMPRESSOR_TARGET_FREQUENCY: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8237, s)),
            CONF_DEVICE_FAN_POWER: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8239, s)),
            CONF_DEVICE_TOTAL_POWER: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x823d, s)),
            CONF_DEVICE_COMPRESSOR_COMMANDED_FREQUENCY: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8236, s)),
            CONF_DEVICE_COMPRESSOR_CURRENT_FREQ: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8238, s)),
            CONF_DEVICE_PHASE_CURRENT: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x82db, s)),
            CONF_DEVICE_PHASE_VOLTAGE: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x82de, s)),
            CONF_DEVICE_PHASE_POWER: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x82df, s)),
            CONF_DEVICE_POWER_FACTOR: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x82e8, s)),
            CONF_DEVICE_SYSTEM_EFFICIENCY: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x82ed, s)),
            CONF_DEVICE_SYSTEM_CAPACITY: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x82f6, s)),
            CONF_DEVICE_TOTAL_ENERGY_CONSUMPTION: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8408, s)),
            CONF_DEVICE_OPERATION_HOURS: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x840b, s)),
            CONF_DEVICE_COMPRESSOR_HOURS: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x840c, s)),
            CONF_DEVICE_FAN_HOURS: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x841a, s)),
            CONF_DEVICE_CONTROL_WATTMETER_UNIT: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8411, s)),
            CONF_DEVICE_WATTMETER_TOTAL_SUM: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8415, s)),
            CONF_DEVICE_WATTMETER_ACCUMULATED: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8416, s)),
            CONF_DEVICE_OPERATION_MODE: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x4001, s)), 
            CONF_DEVICE_OUTDOOR_OPERATION_MODE: (sensor.new_sensor, lambda s: var_dev.add_custom_sensor(0x8001, s)),             
        })

        # Iterate over the actions
        for key, (action, method) in device_actions.items():
            if key in device:
                conf = device[key]
                sens = await action(conf)
                cg.add(method(sens))

        if CONF_DEVICE_ROOM_TEMPERATURE_OFFSET in device:
            cg.add(
                var_dev.set_room_temperature_offset(
                    device[CONF_DEVICE_ROOM_TEMPERATURE_OFFSET]
                )
            )

        if CONF_DEVICE_WATER_TARGET_TEMPERATURE in device:
            conf = device[CONF_DEVICE_WATER_TARGET_TEMPERATURE]
            conf[CONF_UNIT_OF_MEASUREMENT] = UNIT_CELSIUS
            conf[CONF_DEVICE_CLASS] = DEVICE_CLASS_TEMPERATURE
            num = await number.new_number(
                conf, min_value=30.0, max_value=70.0, step=0.5
            )
            cg.add(var_dev.set_target_water_temperature_number(num))

        if CONF_DEVICE_TARGET_TEMPERATURE in device:
            conf = device[CONF_DEVICE_TARGET_TEMPERATURE]
            conf[CONF_UNIT_OF_MEASUREMENT] = UNIT_CELSIUS
            conf[CONF_DEVICE_CLASS] = DEVICE_CLASS_TEMPERATURE
            num = await number.new_number(
                conf, min_value=16.0, max_value=30.0, step=1.0
            )
            cg.add(var_dev.set_target_temperature_number(num))

        if CONF_DEVICE_WATER_OUTLET_TARGET in device:
            conf = device[CONF_DEVICE_WATER_OUTLET_TARGET]
            conf[CONF_UNIT_OF_MEASUREMENT] = UNIT_CELSIUS
            conf[CONF_DEVICE_CLASS] = DEVICE_CLASS_TEMPERATURE
            num = await number.new_number(
                conf, min_value=15.0, max_value=55.0, step=0.1
            )
            cg.add(var_dev.set_water_outlet_target_number(num))

        if CONF_DEVICE_MODE in device:
            conf = device[CONF_DEVICE_MODE]
            values = ["Auto", "Cool", "Dry", "Fan", "Heat"]
            sel = await select.new_select(conf, options=values)
            cg.add(var_dev.set_mode_select(sel))

        if CONF_DEVICE_WATER_HEATER_MODE in device:
            conf = device[CONF_DEVICE_WATER_HEATER_MODE]
            values = ["Eco", "Standard", "Power", "Force"]
            sel = await select.new_select(conf, options=values)
            cg.add(var_dev.set_water_heater_mode_select(sel))

        if CONF_DEVICE_CLIMATE in device:
            conf = device[CONF_DEVICE_CLIMATE]
            var_cli = cg.new_Pvariable(conf[CONF_ID])
            await climate.register_climate(var_cli, conf)
            cg.add(var_dev.set_climate(var_cli))

            # Optional UI mapping: expose Samsung Auto as HA Heat/Cool
            cg.add(
                var_dev.set_map_auto_to_heat_cool(
                    device.get(CONF_MAP_AUTO_TO_HEAT_COOL, False)
                )
            )


        if CONF_DEVICE_CUSTOM in device:
            for cust_sens in device[CONF_DEVICE_CUSTOM]:
                sens = await sensor.new_sensor(cust_sens)
                cg.add(
                    var_dev.add_custom_sensor(
                        cust_sens[CONF_DEVICE_CUSTOM_MESSAGE], sens
                    )
                )

        for key in CUSTOM_SENSOR_KEYS:
            if key in device:
                conf = device[key]
                # combine raw filters with any user-defined filters
                conf_copy = conf.copy()
                conf_copy[CONF_FILTERS] = (
                    conf[CONF_DEVICE_CUSTOM_RAW_FILTERS]
                    if CONF_DEVICE_CUSTOM_RAW_FILTERS in conf
                    else []
                ) + (conf[CONF_FILTERS] if CONF_FILTERS in conf else [])
                sens = await sensor.new_sensor(conf_copy)
                cg.add(
                    var_dev.add_custom_sensor(conf[CONF_DEVICE_CUSTOM_MESSAGE], sens)
                )

        cg.add(var.register_device(var_dev))

    # If debug MQTT is enabled on ESP32, we need ESP-IDF's built-in mqtt component.
    # ESPHome may exclude it by default to reduce compile time, so re-enable it only when needed.
    if CORE.is_esp32 and config[CONF_DEBUG_MQTT_HOST]:
        cg.add_define("SAMSUNG_AC_DEBUG_MQTT")
        try:
            from esphome.components.esp32 import include_builtin_idf_component

            include_builtin_idf_component("mqtt")
        except Exception:
            try:
                from esphome.components.esp32 import include_idf_component

                include_idf_component("mqtt")
            except Exception:
                from esphome.components.esp32 import add_idf_component

                add_idf_component("mqtt")

    cg.add(
        var.set_debug_mqtt(
            config[CONF_DEBUG_MQTT_HOST],
            config[CONF_DEBUG_MQTT_PORT],
            config[CONF_DEBUG_MQTT_USERNAME],
            config[CONF_DEBUG_MQTT_PASSWORD],
        )
    )

    if CONF_DEBUG_LOG_MESSAGES in config:
        cg.add(var.set_debug_log_messages(config[CONF_DEBUG_LOG_MESSAGES]))

    if CONF_DEBUG_LOG_MESSAGES_RAW in config:
        cg.add(var.set_debug_log_messages_raw(config[CONF_DEBUG_LOG_MESSAGES_RAW]))

    if CONF_NON_NASA_KEEPALIVE in config:
        cg.add(var.set_non_nasa_keepalive(config[CONF_NON_NASA_KEEPALIVE]))

    if CONF_DEBUG_LOG_UNDEFINED_MESSAGES in config:
        cg.add(
            var.set_debug_log_undefined_messages(
                config[CONF_DEBUG_LOG_UNDEFINED_MESSAGES]
            )
        )

    # Mapping of config keys to their corresponding methods
    config_actions = {
        CONF_DEBUG_LOG_MESSAGES: var.set_debug_log_messages,
        CONF_DEBUG_LOG_MESSAGES_RAW: var.set_debug_log_messages_raw,
        CONF_NON_NASA_KEEPALIVE: var.set_non_nasa_keepalive,
        CONF_DEBUG_LOG_UNDEFINED_MESSAGES: var.set_debug_log_undefined_messages,
    }

    # Iterate over the actions
    for key, method in config_actions.items():
        if key in config:
            cg.add(method(config[key]))

    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
