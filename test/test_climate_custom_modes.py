"""Exercise production climate codegen at the entity-registration boundary."""

import importlib.util
import os
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import AsyncMock, patch

import yaml


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "samsung_ac_component", ROOT / "components/samsung_ac/__init__.py"
)
COMPONENT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COMPONENT)


class Entity:
    def __init__(self, name):
        self.name = name

    def __getattr__(self, method):
        return lambda *args: (self.name, method, args)


class ClimateCustomModesTest(unittest.IsolatedAsyncioTestCase):
    async def generate(self, capabilities=None, *, devices=None, global_capabilities=None):
        if devices is None:
            devices = [self.device("unit", capabilities)]
        config = {
            "id": "bus",
            "devices": devices,
            "debug_mqtt_host": "",
            "debug_mqtt_port": 1883,
            "debug_mqtt_username": "",
            "debug_mqtt_password": "",
        }
        if global_capabilities is not None:
            config["capabilities"] = global_capabilities
        events = []

        async def register(entity, conf):
            events.append((entity.name, "register_climate", ()))

        with (
            patch.object(COMPONENT, "CORE", SimpleNamespace(
                is_esp8266=False, is_libretiny=False, is_esp32=False
            )),
            patch.object(COMPONENT.cg, "new_Pvariable", side_effect=lambda name, *args: Entity(name)),
            patch.object(COMPONENT.cg, "add", side_effect=events.append),
            patch.object(COMPONENT.cg, "register_component", new=AsyncMock()),
            patch.object(COMPONENT.uart, "register_uart_device", new=AsyncMock()),
            patch.object(COMPONENT.climate, "register_climate", side_effect=register),
        ):
            await COMPONENT.to_code(config)
        return events

    @staticmethod
    def device(name, capabilities=None):
        device = {
            "samsung_ac_device_id": name,
            "address": "20.00.01",
            "climate": {"id": name + "_climate"},
        }
        if capabilities is not None:
            device["capabilities"] = capabilities
        return device

    def lists(self, events, name="unit"):
        entity = name + "_climate"
        result = {}
        registration = events.index((entity, "register_climate", ()))
        for method in ("set_supported_custom_fan_modes", "set_supported_custom_presets"):
            calls = [(index, event) for index, event in enumerate(events)
                     if event[:2] == (entity, method)]
            self.assertEqual(len(calls), 1, method + " must be initialized once")
            index, event = calls[0]
            self.assertLess(index, registration)
            result[method] = event[2][0]
        configured = [index for index, event in enumerate(events)
                      if event[0] == name and event[1] in (
                          "set_supports_fan_modes", "set_supports_turbo_mode", "add_alt_mode", "set_climate"
                      )]
        first_list = min(index for index, event in enumerate(events)
                         if event[0] == entity and event[1].startswith("set_supported_custom_"))
        self.assertTrue(all(index < first_list for index in configured))
        return result["set_supported_custom_fan_modes"], result["set_supported_custom_presets"]

    async def test_turbo_capability_filtering(self):
        for capabilities, expected in (
            ({}, ["Turbo"]),
            ({"turbo_mode": True}, ["Turbo"]),
            ({"turbo_mode": False}, []),
            ({"fan_modes": False, "turbo_mode": True}, []),
            ({"fan_modes": False, "turbo_mode": False}, []),
        ):
            with self.subTest(capabilities=capabilities):
                fan_modes, presets = self.lists(await self.generate(capabilities))
                self.assertEqual(fan_modes, expected)
                self.assertEqual(presets, [])

    async def test_default_custom_and_standard_presets(self):
        capabilities = {"presets": {name: True for name in COMPONENT.PRESETS}}
        events = await self.generate(capabilities)
        _, custom = self.lists(events)
        self.assertEqual(custom, ["Quiet", "Fast", "LongReach", "WindFree"])
        modes = [event[2] for event in events if event[:2] == ("unit", "add_alt_mode")]
        self.assertEqual(modes, [
            ("None", 0), ("Sleep", 1), ("Quiet", 2), ("Fast", 3),
            ("LongReach", 6), ("Eco", 7), ("WindFree", 9),
        ])

    async def test_custom_names_and_values_are_preserved(self):
        names = ["Night mode", 'Quiet "room"', "ſleep", "Long reach " * 20, "Özel Eco", "WindFree"]
        presets = {key: {"enabled": True, "name": name, "value": 40 + index}
                   for index, (key, name) in enumerate(zip(COMPONENT.PRESETS, names))}
        events = await self.generate({"presets": presets})
        self.assertEqual(self.lists(events)[1], names)
        modes = [event[2] for event in events if event[:2] == ("unit", "add_alt_mode")]
        self.assertEqual(modes, [("None", 0)] + [(name, 40 + index) for index, name in enumerate(names)])

    async def test_standard_preset_names_remain_standard_case_insensitively(self):
        for name in ("None", "eCo", "Away", "Boost", "Comfort", "Home", "Sleep", "Activity"):
            with self.subTest(name=name):
                events = await self.generate({"presets": {"quiet": {"enabled": True, "name": name}}})
                self.assertEqual(self.lists(events)[1], [])
                self.assertIn(("unit", "add_alt_mode", (name, 2)), events)

    async def test_disabled_presets_are_not_advertised(self):
        events = await self.generate({"presets": {
            "quiet": False, "fast": {"enabled": False, "name": "Disabled"},
        }})
        self.assertEqual(self.lists(events)[1], [])
        self.assertFalse(any(event[1] == "add_alt_mode" for event in events))

    async def test_global_capabilities_and_device_override(self):
        events = await self.generate(devices=[self.device("first"), self.device("second", {})],
                                     global_capabilities={"turbo_mode": False, "presets": {"quiet": True}})
        self.assertEqual(self.lists(events, "first"), ([], ["Quiet"]))
        self.assertEqual(self.lists(events, "second"), (["Turbo"], []))

    async def test_device_lists_are_independent(self):
        events = await self.generate(devices=[
            self.device("first", {"presets": {"quiet": {"enabled": True, "name": "First room"}}}),
            self.device("second", {"turbo_mode": False, "presets": {"fast": True}}),
        ])
        self.assertEqual(self.lists(events, "first"), (["Turbo"], ["First room"]))
        self.assertEqual(self.lists(events, "second"), ([], ["Fast"]))

    async def test_no_climate_entity(self):
        device = self.device("unit", {"presets": {"quiet": True}})
        del device["climate"]
        events = await self.generate(devices=[device])
        self.assertFalse(any(event[1].startswith("set_supported_custom_") for event in events))


class GeneratedClimateModesTest(unittest.TestCase):
    def test_literals_and_initialization_order_in_generated_firmware(self):
        workspace = ROOT / ".esphome" / "climate-custom-modes-test"
        workspace.mkdir(parents=True, exist_ok=True)
        config = {
            "esphome": {"name": "climate-custom-modes-test", "build_path": str(workspace / "build")},
            "esp32": {"board": "m5stack-atom", "framework": {"type": "esp-idf"}},
            "external_components": [{"source": {"type": "local", "path": str(ROOT / "components")}}],
            "uart": {"tx_pin": 19, "rx_pin": 22, "baud_rate": 9600, "parity": "EVEN"},
            "samsung_ac": {"devices": [
                {"address": "20.00.01", "climate": {"id": "turbo_climate", "name": "Turbo"},
                 "capabilities": {"presets": {"sleep": True, "eco": True,
                     "quiet": {"enabled": True, "name": 'Quiet "room"'}, "fast": True}}},
                {"address": "20.00.02", "climate": {"id": "no_turbo_climate", "name": "No Turbo"},
                 "capabilities": {"turbo_mode": False}},
                {"address": "20.00.03", "climate": {"id": "no_fan_climate", "name": "No Fan"},
                 "capabilities": {"fan_modes": False, "turbo_mode": True}},
                {"address": "00", "climate": {"id": "non_nasa_climate", "name": "Non NASA"},
                 "capabilities": {"presets": {"longreach": True, "windfree": True}}},
            ]},
        }
        config_path = workspace / "config.yaml"
        config_path.write_text(yaml.safe_dump(config, sort_keys=False), encoding="utf-8")
        env = os.environ.copy()
        env["TEMP"] = env["TMP"] = str(workspace)
        result = subprocess.run(
            [sys.executable, "-m", "esphome", "compile", "--only-generate", str(config_path)],
            cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, encoding="utf-8", errors="replace", timeout=120,
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        source = (workspace / "build/src/main.cpp").read_text(encoding="utf-8")
        for entity, fan_list, preset_list in (
            ("turbo_climate", '{"Turbo"}', r'{"Quiet \042room\042", "Fast"}'),
            ("no_turbo_climate", "{}", "{}"),
            ("no_fan_climate", "{}", "{}"),
            ("non_nasa_climate", '{"Turbo"}', '{"LongReach", "WindFree"}'),
        ):
            with self.subTest(entity=entity):
                registration = source.index(f"App.register_climate({entity}")
                for method, value in (
                    ("set_supported_custom_fan_modes", fan_list),
                    ("set_supported_custom_presets", preset_list),
                ):
                    call = f"{entity}->{method}({value});"
                    self.assertEqual(source.count(call), 1)
                    self.assertLess(source.index(call), registration)


if __name__ == "__main__":
    unittest.main()
