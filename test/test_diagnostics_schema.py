"""Validate the diagnostics flag and its generated C++ with ESPHome."""

import os
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[1]


class DiagnosticsSchemaTest(unittest.TestCase):
    def setUp(self):
        workspace = ROOT / ".esphome"
        workspace.mkdir(exist_ok=True)
        self.workspace = Path(
            self.enterContext(TemporaryDirectory(prefix="diagnostics-schema-", dir=workspace))
        )

    def run_esphome(self, *command, **options):
        config = {
            "esphome": {
                "name": "diagnostics-schema-test",
                "build_path": str(self.workspace / "build"),
            },
            "esp32": {"board": "m5stack-atom", "framework": {"type": "esp-idf"}},
            "external_components": [
                {"source": {"type": "local", "path": str(ROOT / "components")}}
            ],
            "uart": {"tx_pin": 19, "rx_pin": 22, "baud_rate": 9600, "parity": "EVEN"},
            "samsung_ac": {"id": "hvac_bus", "devices": [{"address": "20.00.01"}], **options},
        }
        path = self.workspace / "config.yaml"
        path.write_text(yaml.safe_dump(config), encoding="utf-8")
        # Each invocation gets fresh ESPHome CORE state.
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
        return yaml.safe_load(result.stdout)["samsung_ac"]

    def test_omitted_flag_defaults_to_false(self):
        config = self.validated_config()
        self.assertIs(config.get("debug_log_diagnostics"), False)

    def test_accepts_explicit_booleans(self):
        for value in (True, False):
            with self.subTest(value=value):
                config = self.validated_config(debug_log_diagnostics=value)
                self.assertIs(config.get("debug_log_diagnostics"), value)

    def test_rejects_non_boolean_string(self):
        result = self.run_esphome("config", debug_log_diagnostics="not-a-boolean")
        output = result.stdout + result.stderr
        self.assertNotEqual(result.returncode, 0, output)
        self.assertIn("Expected boolean value", output)

    def test_generates_configured_setter_call(self):
        for value, literal in ((True, "true"), (False, "false")):
            with self.subTest(value=value):
                result = self.run_esphome(
                    "compile", "--only-generate", debug_log_diagnostics=value
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                source = (self.workspace / "build/src/main.cpp").read_text(encoding="utf-8")
                calls = re.findall(r"hvac_bus->set_debug_log_diagnostics\(([^)]*)\);", source)
                self.assertEqual(calls, [literal])


if __name__ == "__main__":
    unittest.main()
