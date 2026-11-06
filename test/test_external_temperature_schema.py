"""Experimental external source schema gates, using real ESPHome validation."""
import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import unittest
import esphome.config_validation as cv
import yaml

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("samsung_ac_component", ROOT / "components/samsung_ac/__init__.py")
C = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(C)


class ExternalSchemaTest(unittest.TestCase):
    def config(self):
        C.CORE.reset()
        return {"address": "20.00.01", "external_room_temperature": {
            "experimental": True, "acknowledge_latched_temperature": True,
            "controller_absent_confirmed": True, "sensor": "office_source",
            "source_address": "50.00.00", "enable_switch": {"name": "Office experimental sender"},
        }}

    def test_absent_is_unchanged(self):
        for address in ("00", "20.00.01"):
            self.assertNotIn("external_room_temperature", C.DEVICE_SCHEMA({"address": address}))

    def test_defaults_are_nonrestoring(self):
        e = C.DEVICE_SCHEMA(self.config())["external_room_temperature"]
        self.assertEqual(e["enable_switch"]["restore_mode"], "DISABLED")
        self.assertEqual(e["update_interval"].total_milliseconds, 45000)
        self.assertEqual(e["max_age"].total_milliseconds, 120000)

    def test_requires_explicit_acknowledgements(self):
        for key in ("experimental", "acknowledge_latched_temperature", "controller_absent_confirmed"):
            for absent in (True, False):
                conf = self.config()
                if absent:
                    del conf["external_room_temperature"][key]
                else:
                    conf["external_room_temperature"][key] = False
                with self.subTest(key=key, absent=absent), self.assertRaises(cv.Invalid):
                    C.DEVICE_SCHEMA(conf)

    def test_individual_nasa_indoor_only(self):
        for address in ("00", "10.00.00", "20.ff.01", "20.00.ff", "20.00.001", "20.gg.00"):
            conf = self.config()
            conf["address"] = address
            with self.subTest(address=address), self.assertRaises(cv.Invalid):
                C.DEVICE_SCHEMA(conf)

    def test_source_class_and_broadcast_rejection(self):
        for address in ("80.00.00", "62.00.64", "50.ff.00", "50.00.ff", "50.0.0"):
            conf = self.config()
            conf["external_room_temperature"]["source_address"] = address
            with self.subTest(address=address), self.assertRaises(cv.Invalid):
                C.DEVICE_SCHEMA(conf)

    def test_timing_and_numeric_bounds(self):
        for key, value in (("update_interval", "1s"), ("update_interval", "301s"),
                           ("response_timeout", "11s"), ("verification_timeout", "44s"),
                           ("observe_for", "29s"), ("max_age", "601s"), ("max_age", "30s"),
                           ("minimum", 40), ("maximum", 61), ("max_step", 21), ("tolerance", 0),
                           ("minimum", float("nan")), ("tolerance", float("inf"))):
            conf = self.config()
            conf["external_room_temperature"][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(cv.Invalid):
                C.DEVICE_SCHEMA(conf)

    def test_no_restore_or_inversion(self):
        for key, value in (("restore_mode", "RESTORE_DEFAULT_ON"), ("inverted", True)):
            conf = self.config()
            conf["external_room_temperature"]["enable_switch"][key] = value
            with self.subTest(key=key), self.assertRaises(cv.Invalid):
                C.DEVICE_SCHEMA(conf)

    def test_feedback_rejected(self):
        for key in ("room_temperature", "input_temperature", "sent_temperature", "reported_temperature", "failures"):
            conf = self.config()
            if key == "room_temperature":
                conf[key] = {"id": "office_source", "name": "Room"}
            else:
                conf["external_room_temperature"][key] = {"id": "office_source", "name": "Diagnostic"}
            with self.subTest(key=key), self.assertRaises(cv.Invalid):
                C.DEVICE_SCHEMA(conf)

    def test_unknown_fallback_options_rejected(self):
        for key in ("auto_enable", "fallback_temperature", "power_off_on_failure", "sensor_selection", "retries"):
            conf = self.config()
            conf["external_room_temperature"][key] = True
            with self.subTest(key=key), self.assertRaises(cv.Invalid):
                C.DEVICE_SCHEMA(conf)

    def test_cross_device_feedback_and_duplicate_target(self):
        first = C.DEVICE_SCHEMA(self.config())
        second = C.DEVICE_SCHEMA({"address": "20.00.02", "room_temperature": {"id": "office_source", "name": "Other unit"}})
        with self.assertRaises(cv.Invalid):
            C.validate_external_devices({"devices": [first, second]})
        duplicate = C.DEVICE_SCHEMA({"address": "20.00.01"})
        with self.assertRaises(cv.Invalid):
            C.validate_external_devices({"devices": [first, duplicate]})


class ExternalDocumentationTest(unittest.TestCase):
    def generate(self, native):
        doc = (ROOT / "docs_src/wiki/External-Room-Temperature.md").read_text(encoding="utf-8")
        blocks = re.findall(r"```yaml\n(.*?)```", doc, re.S)
        config = yaml.safe_load(blocks[0])
        if native:
            config.update(yaml.safe_load(blocks[1]))
        name = "external-native-example" if native else "external-ha-example"
        scratch = ROOT / ".esphome/issue338"
        scratch.mkdir(parents=True, exist_ok=True)
        config.update({
            "esphome": {"name": name, "build_path": str(scratch / (name + "-build"))},
            "esp32": {"board": "m5stack-atom", "framework": {"type": "esp-idf"}},
            "api": {}, "wifi": {"ap": {"ssid": "CompileTest", "password": "compile-only"}},
            "uart": {"tx_pin": "GPIO19", "rx_pin": "GPIO22", "baud_rate": 9600, "parity": "EVEN"},
            "external_components": [{"source": {"type": "local", "path": str(ROOT / "components")}}],
        })
        path = scratch / (name + ".yaml")
        path.write_text(yaml.safe_dump(config, sort_keys=False), encoding="utf-8")
        result = subprocess.run([sys.executable, "-m", "esphome", "compile", str(path), "--only-generate"],
                                cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=120)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        generated = (scratch / (name + "-build") / "src/main.cpp").read_text(encoding="utf-8")
        self.assertIn('"20.00.01", "50.00.00"', generated)
        self.assertIn("->set_timing(45000, 120000, 60000, 5000, 90000)", generated)
        self.assertNotIn('"20.00.02", "50.', generated)
        self.assertNotIn('"20.00.00", "50.', generated)
        self.assertNotIn("->turn_on()", generated)

    def test_home_assistant_example_generates(self):
        self.generate(False)

    def test_native_sensor_example_generates(self):
        self.generate(True)


if __name__ == "__main__":
    unittest.main()
