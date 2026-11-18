# Coding style

Follow these conventions for new code. In existing files, match the surrounding style and keep unrelated formatting out of the diff.

## Project layout

- `components/samsung_ac/__init__.py`: YAML validation and ESPHome code generation.
- `components/samsung_ac/samsung_ac.cpp`: UART I/O, frame buffering, and the send queue.
- `components/samsung_ac/samsung_ac_device.cpp`: device state and ESPHome entities. Keep enum mappings in `conversions.cpp`.
- `components/samsung_ac/protocol.cpp`: shared protocol detection and dispatch. Keep wire formats and protocol-specific behavior in `protocol_nasa.*` or `protocol_non_nasa.*`.
- `example.yaml`: user-facing configuration example. Edit the wiki sources under `docs_src/wiki/`, not generated HTML under `docs/`.

## Formatting and naming

- Follow [`.editorconfig`](.editorconfig): UTF-8, LF, spaces, and a final newline. Use four spaces in Python and two in YAML.
- **C++:** Match local indentation and braces. `samsung_ac*.cpp` mostly uses two spaces per level; `protocol*.cpp` mostly uses four. Opening braces usually go on their own line.
- **C++ names:** Use `snake_case` for new functions and variables, `PascalCase` for new types, and `UPPER_SNAKE_CASE` for named protocol constants such as `KEEPALIVE_DELAY_MS`. Keep existing names such as `Samsung_AC` that Python code generation references.
- **Python and YAML names:** Use `snake_case` for functions, variables, and YAML keys. Name Python constants for configuration keys `CONF_*`, as in `CONF_NON_NASA_TX_DELAY_MS` for `non_nasa_tx_delay_ms`.
- **YAML examples:** Quote device addresses so YAML preserves them as strings, especially `"00"`. Use placeholders, not real credentials.

## Protocol changes

- Use explicit widths for wire fields, such as `uint8_t` and `uint16_t`. Include units in names for timing and measurements (`_ms`, `_w`, `_kwh`).
- Keep protocol code behind `ProtocolRequest` and `MessageTarget` where applicable. Do not mix NASA and Non-NASA wire rules in the entity code.
- Comment why a timing limit, message ID, or model-specific workaround exists. Reuse the logging helpers in `log.h` and keep raw frame logging behind its debug flag.
- Treat YAML keys, defaults, and entity behavior as user-facing. Update the example or wiki when they change, and document any protocol restrictions.

## Before opening a pull request

Add a regression case when changing behavior. Use the C++ tests for frames and send queues, and the Python tests for schema and code generation.

Run the C++ tests from the repository root with a C++17-capable `g++`. Run them one at a time because they share `test.exe`:

```sh
sh test/test_nasa.sh
sh test/build_and_run.sh test/main_test_nasa_display.cpp
sh test/test_non_nasa_queue.sh
```

Known issue: `sh test/test_non_nasa.sh` fails to compile because some assertions still compare `Temperature` values to integers. The queue test runs separately.

With ESPHome and PyYAML installed, run the Python tests from the same directory:

```sh
python3 -m unittest discover -s test -p 'test_*.py'
```

CI builds firmware for Arduino and ESP-IDF. These builds do not replace the local tests. For bus timing or hardware-specific changes, report the model, protocol, and what you tested on hardware.
