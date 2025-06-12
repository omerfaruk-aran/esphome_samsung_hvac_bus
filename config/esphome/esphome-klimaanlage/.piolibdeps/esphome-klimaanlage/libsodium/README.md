# libsodium-esphome

This is a port of [libsodium](https://github.com/jedisct1/libsodium) - A modern, portable, easy to use crypto library.

We use libsodium 1.0.18 as the base (see libsodium submodule) with some simple patches in port/port_include to make the library compile with the [platformio](https://platformio.org/) build system.

Only a subset of libsodium is compiled, namely the cryptographic primitives required for [noise-c](https://github.com/esphome/noise-c/).

libsodium is licensed under the [ISC License](https://github.com/jedisct1/libsodium#license)
