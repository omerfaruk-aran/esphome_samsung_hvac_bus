"""Validate the opt-in telemetry entities and their generated bindings."""

import os
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[1]


class _ConfigLoader(yaml.SafeLoader):
    pass


_ConfigLoader.add_constructor("!lambda", yaml.SafeLoader.construct_scalar)


class TelemetrySchemaTest(unittest.TestCase):
    def setUp(self):
        self.workspace = Path(self.enterContext(TemporaryDirectory(prefix="telemetry-")))

    def run_esphome(self, *command, **options):
        config = {
            "esphome": {
                "name": "telemetry-schema-test",
                "build_path": str(self.workspace / "build"),
            },
            "esp32": {"board": "m5stack-atom", "framework": {"type": "esp-idf"}},
            "external_components": [
                {"source": {"type": "local", "path": str(ROOT / "components")}}
            ],
            "uart": {"tx_pin": 19, "rx_pin": 22, "baud_rate": 9600, "parity": "EVEN"},
            "samsung_ac": {"devices": [], **options},
        }
        path = self.workspace / "config.yaml"
        path.write_text(yaml.safe_dump(config), encoding="utf-8")
        return subprocess.run(
            [sys.executable, "-m", "esphome", *command, str(path)],
            cwd=self.workspace,
            env={**os.environ, "ESPHOME_DATA_DIR": str(self.workspace)},
            capture_output=True,
            encoding="utf-8",
            timeout=120,
        )

    def validated_config(self, **options):
        result = self.run_esphome("config", **options)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return yaml.load(result.stdout, Loader=_ConfigLoader)["samsung_ac"]

    def test_omitted_and_disabled_do_not_create_entities(self):
        devices = [{"address": "10.00.00"}, {"address": "20.00.00"}]
        for options in ({}, {"telemetry": False}):
            with self.subTest(options=options):
                config = self.validated_config(devices=devices, **options)
                self.assertIs(config.get("telemetry"), False)
                outdoor, indoor = config["devices"]
                self.assertNotIn("outdoor_temperature", outdoor)
                self.assertNotIn("indoor_eva_in_temperature", indoor)
                self.assertEqual(outdoor["custom_sensor"], [])
                self.assertEqual(indoor["custom_sensor"], [])

    def test_enabled_fills_only_configured_nasa_indoor_and_outdoor_devices(self):
        config = self.validated_config(
            telemetry=True,
            devices=[
                {"address": address}
                for address in ("10.00.00", "20.00.00", "20.00.01", "00", "50.00.00", "20.bad")
            ],
        )
        outdoor, indoor_a, indoor_b, *other = config["devices"]
        self.assertFalse(config["debug_log_diagnostics"])
        self.assertEqual(
            {key for key in outdoor if key.startswith("outdoor_")},
            {
                "outdoor_temperature", "outdoor_instantaneous_power",
                "outdoor_cumulative_energy", "outdoor_operation_heatcool",
            },
        )
        self.assertEqual(
            [
                (int(str(item["message"]), 0), item["unit_of_measurement"])
                for item in outdoor["custom_sensor"]
            ],
            [(0x8238, "Hz"), (0x823D, "rpm")],
        )
        self.assertEqual(
            outdoor["outdoor_cumulative_energy"]["filters"][-1],
            {"throttle": "10s"},
        )
        self.assertIn(
            "lambda", outdoor["outdoor_operation_heatcool"]["filters"][0]
        )
        for item in outdoor["custom_sensor"]:
            self.assertEqual(item["filters"], [{"throttle": "10s"}])
        for indoor in (indoor_a, indoor_b):
            for key in ("indoor_eva_in_temperature", "indoor_eva_out_temperature"):
                self.assertEqual(indoor[key]["filters"], [{"throttle": "10s"}])
            self.assertIn("filter_warning", indoor)
            self.assertEqual(indoor["custom_sensor"][0]["type"], "binary")
            self.assertEqual(int(str(indoor["custom_sensor"][0]["message"]), 0), 0x4028)
        self.assertNotEqual(
            indoor_a["filter_warning"]["name"], indoor_b["filter_warning"]["name"]
        )
        for device in other:
            self.assertEqual(device["custom_sensor"], [])
            self.assertNotIn("filter_warning", device)
            self.assertNotIn("outdoor_temperature", device)

    def test_existing_configuration_takes_precedence(self):
        config = self.validated_config(
            telemetry=True,
            devices=[
                {
                    "address": "10.00.00",
                    "outdoor_temperature": {"name": "My outdoor temperature"},
                    "custom_sensor": [
                        {"name": "My Hz", "message": "0x8238"},
                        {"name": "My power", "message": 0x8413},
                    ],
                },
                {
                    "address": "20.00.0A",
                    "custom_sensor": [
                        {"name": "My filter", "message": 0x4027, "type": "binary"},
                        {"name": "My demand", "message": 0x4028, "type": "binary"},
                    ],
                },
            ],
        )
        outdoor, indoor = config["devices"]
        self.assertEqual(outdoor["outdoor_temperature"]["name"], "My outdoor temperature")
        self.assertFalse(outdoor["outdoor_temperature"].get("filters"))
        self.assertNotIn("outdoor_instantaneous_power", outdoor)
        self.assertEqual(len(outdoor["custom_sensor"]), 3)
        self.assertEqual(outdoor["custom_sensor"][0]["name"], "My Hz")
        self.assertEqual(indoor["address"], "20.00.0a")
        self.assertNotIn("filter_warning", indoor)
        self.assertEqual(len(indoor["custom_sensor"]), 2)

    def test_named_message_overrides_take_precedence(self):
        result = self.run_esphome(
            "compile", "--only-generate",
            telemetry=True,
            devices=[
                {
                    "address": "10.00.00",
                    "water_temperature": {
                        "id": "my_water",
                        "name": "My water",
                        "message": "0x8238",
                    },
                },
                {
                    "address": "10.00.01",
                    "room_humidity": {
                        "id": "my_humidity",
                        "name": "My humidity",
                        "message": 0x1823D,
                    },
                },
            ],
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        source = (self.workspace / "build/src/main.cpp").read_text(encoding="utf-8")
        registrations = {}
        explicit = {}
        for device, message, sensor_id in re.findall(
            r"(\w+)->add_custom_sensor\(([^,]+), (\w+)\);", source
        ):
            message = int(message, 0) & 0xFFFF
            registrations.setdefault(device, []).append(message)
            if sensor_id in ("my_water", "my_humidity"):
                explicit[sensor_id] = message
        self.assertEqual(len(registrations), 2)
        for messages in registrations.values():
            self.assertCountEqual(messages, [0x8238, 0x823D])
        self.assertEqual(explicit, {"my_water": 0x8238, "my_humidity": 0x823D})

    def test_generated_energy_converts_unsigned_wh(self):
        config = self.validated_config(
            telemetry=True,
            devices=[
                {"address": "10.00.00"},
                {
                    "address": "10.00.01",
                    "outdoor_cumulative_energy": {"name": "My energy"},
                },
            ],
        )
        generated, explicit = config["devices"]
        self.assertEqual(explicit["outdoor_cumulative_energy"]["filters"], [{"multiply": 0.001}])
        scale = generated["outdoor_cumulative_energy"]["filters"][0]
        body = scale["lambda"] if "lambda" in scale else f"return x * {scale['multiply']}f;"
        executable = self.workspace / "energy-filter"
        compiled = subprocess.run(
            ["g++", "-std=c++17", "-x", "c++", "-", "-o", str(executable)],
            input=(
                "#include <iomanip>\n#include <iostream>\n#include <optional>\n"
                f"std::optional<float> convert(float x) {{ {body} }}\n"
                "int main() {\n"
                "  for (float wh : {1009768.0f, -2147483648.0f, -1.0f})\n"
                "    std::cout << std::setprecision(12) << convert(wh).value() << '\\n';\n"
                "}\n"
            ),
            capture_output=True,
            encoding="utf-8",
            timeout=30,
        )
        self.assertEqual(compiled.returncode, 0, compiled.stderr)
        result = subprocess.check_output([str(executable)], encoding="utf-8", timeout=30)
        normal, sign_bit, maximum = map(float, result.splitlines())
        self.assertAlmostEqual(normal, 1009.768, delta=0.0001)
        self.assertAlmostEqual(sign_bit, 2147483.648, delta=0.125)
        self.assertAlmostEqual(maximum, 4294967.295, delta=0.25)

    def test_invalid_flag_is_rejected(self):
        result = self.run_esphome("config", telemetry="not-a-boolean")
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("Expected boolean value", result.stdout + result.stderr)

    def test_generates_read_only_entity_bindings(self):
        result = self.run_esphome(
            "compile", "--only-generate",
            telemetry=True,
            devices=[{"address": "10.00.00"}, {"address": "20.00.01"}],
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        source = (self.workspace / "build/src/main.cpp").read_text(encoding="utf-8")
        for binding in (
            "set_outdoor_cumulative_energy_sensor(",
            "set_outdoor_operation_heatcool_text_sensor(",
            "set_indoor_eva_in_temperature_sensor(",
        ):
            self.assertEqual(source.count(binding), 1, binding)
        numeric = re.findall(r"->add_custom_sensor\(([^,]+),", source)
        binary = re.findall(r"->add_custom_binary_sensor\(([^,]+),", source)
        self.assertCountEqual([int(value, 0) for value in numeric], [0x8238, 0x823D])
        self.assertCountEqual([int(value, 0) for value in binary], [0x4027, 0x4028])
        self.assertNotIn("App.register_switch(", source)
