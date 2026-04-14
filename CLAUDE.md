# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is `smartmet-plugin-admin`, a SmartMet Server plugin that exposes administrative and monitoring HTTP endpoints. The plugin itself is a thin pass-through (~250 lines total): it handles HTTP basic auth and delegates all query logic to `Spine::Reactor::executeAdminRequest()`. The plugin is **deprecated** — maintained for compatibility but not under active development.

## Build and development

```bash
make                # Build admin.so
make clean          # Clean build artifacts
make format         # Run clang-format
make test           # Run integration tests (requires smartmet-plugin-test and engines installed)
make rpm            # Build RPM package
```

The plugin compiles to a single `admin.so` shared library. It links against `smartmet-spine` and `smartmet-macgyver`, and requires `configpp` (libconfig).

## Testing

Tests are **integration tests**, not unit tests. They use `smartmet-plugin-test` to start a SmartMet Server instance, send HTTP requests from `test/input/*.get`, and compare responses against expected output in `test/output/`.

```bash
cd test
smartmet-plugin-test --handler=/admin --reactor-config=cnf/reactor.conf
```

In CI, a local geonames PostgreSQL database is created and `.expect_to_fail` tests are skipped. Locally, tests expect an already-running geonames database at the host configured in `test/cnf/geonames.conf.in`.

## Architecture

**Plugin loading pattern:** The server uses `dlopen` to load `admin.so` at runtime. Two `extern "C"` entry points in `Plugin.cpp` — `create()` and `destroy()` — serve as the plugin lifecycle interface.

**Request flow:**
1. Server routes requests for `/admin` to the plugin's content handler
2. Plugin calls `Spine::Reactor::executeAdminRequest()` with an auth callback
3. Reactor authenticates via the plugin's `authenticateRequest()` method (HTTP basic auth)
4. Reactor processes the `what=` query parameter and returns results

**Key classes:**
- `Plugin` (in `admin/Plugin.h`) — inherits `SmartMetPlugin` and `Spine::HTTP::Authentication`

**Configuration:** Single libconfig file requiring `user` and `password` keys for HTTP basic auth. Sample at `cnf/admin.conf.sample`.

## Dependencies

Runtime engines (not linked directly — resolved by the server at load time): `geonames`, `querydata`, `grid`, `observation`, `contour`, `sputnik`. These engines provide the data that admin queries report on.
