"""Validate the optional NASA display schema with the installed ESPHome version."""

import importlib.util
from pathlib import Path
import unittest

import esphome.config_validation as cv

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "samsung_ac_component", ROOT / "components/samsung_ac/__init__.py"
)
COMPONENT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COMPONENT)


class DisplaySchemaTest(unittest.TestCase):
    def test_absent_remains_absent(self):
        for address in ("00", "20.00.02"):
            config = COMPONENT.DEVICE_SCHEMA({"address": address})
            self.assertNotIn("display_light", config)

    def test_nasa_indoor(self):
        config = COMPONENT.DEVICE_SCHEMA(
            {
                "address": "20.0A.FF",
                "display_light": {
                    "name": "Display",
                    "state_valid": {"name": "Display State Valid"},
                },
            }
        )
        self.assertIn("state_valid", config["display_light"])
        self.assertEqual(config["address"], "20.0a.ff")
        self.assertEqual(config["display_light"]["restore_mode"], "DISABLED")

    def test_optional_manual_refresh(self):
        config = COMPONENT.DEVICE_SCHEMA(
            {
                "address": "20.00.01",
                "display_light": {
                    "name": "Bedroom Display Light",
                    "state_valid": {"name": "Display State Valid"},
                    "refresh": {"name": "Bedroom Refresh Display Light"},
                },
            }
        )
        self.assertIn("refresh", config["display_light"])
        self.assertIn("state_valid", config["display_light"])

        without_button = COMPONENT.DEVICE_SCHEMA(
            {"address": "20.00.01", "display_light": {"name": "Display"}}
        )
        self.assertNotIn("refresh", without_button["display_light"])

    def test_reject_other_addresses(self):
        for address in ("00", "10.00.00", "30.00.00", "20.00", "20.gg.00", "20.00.001"):
            with self.subTest(address=address), self.assertRaises(cv.Invalid):
                COMPONENT.DEVICE_SCHEMA(
                    {"address": address, "display_light": {"name": "Display"}}
                )

    def test_no_inversion(self):
        with self.assertRaises(cv.Invalid):
            COMPONENT.DEVICE_SCHEMA(
                {"address": "20.00.02", "display_light": {"name": "Display", "inverted": True}}
            )

    def test_existing_features_still_validate(self):
        config = COMPONENT.DEVICE_SCHEMA(
            {
                "address": "20.00.02",
                "power": {"name": "Power"},
                "automatic_cleaning": {"name": "Clean"},
                "silence_mode": {"name": "Silence"},
                "filter_reset": {"name": "Reset Filter"},
                "climate": {"name": "Climate"},
            }
        )
        self.assertNotIn("display_light", config)


if __name__ == "__main__":
    unittest.main()
