Any contribution that you make to this repository will
be under the MIT license, as dictated by that
[license](https://opensource.org/licenses/MIT).

## Quick checks

The quick checks require Git, Python 3.10 or newer, and a C++17 compiler;
they do not require ROS or Docker. From the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install pre-commit==4.2.0
pre-commit install
pre-commit run --all-files
```

The first run downloads the pinned hook environment. The commit hook checks
staged files and runs the standalone model tests when model-related files
change. Run `tools/run-model-tests.sh` directly for those tests at any time;
`CXX` selects the compiler and `CXXFLAGS` supplies whitespace-separated flags.
Assertions remain enabled even if `CXXFLAGS` defines `NDEBUG`.

CI runs these checks before its ROS distribution matrix. The full builds and
installed GUI smoke tests require Docker and the dependencies described in
the README; quick checks are not a replacement for those integration tests.

## GUI validation

The [monitor guide](docs/monitor.md) describes the expected user-facing behavior.
Keep **MESSAGE IDS** as a permanent right-hand checkbox list and **FRAME METADATA**
horizontal beneath the main table. Do not move FD payloads into the metadata
panel: every visible FD row must retain its complete inline 8×8 matrix.

### Focused regression tests

After building and sourcing the ROS workspace described in the
[README](README.md#build), run these commands from that workspace root:

```sh
colcon test --packages-select decanstructor2 --return-code-on-test-failure \
  --ctest-args -R '^window_test$'
colcon test-result --verbose
```

CTest supplies Xvfb for `window_test`; do not wrap that CTest command in another
Xvfb session. GUI regressions should drive actual buttons and checkboxes, verify
the resulting rows and selected metadata, and reacquire widgets after sorting
or filtering rebuilds them. Check both horizontal and vertical byte-cell bounds
at 1280×800 and 1000×600, including the last byte in each visible FD matrix.
Keep the checklist and table widget counts viewport-bounded at the message limit.

### Installed-app smoke and screenshots

The Docker CI entrypoint, `tools/run-tests.sh`, builds and tests the packages,
then runs `test/smoke_monitor.py` against the installed executable. To run just
that smoke in an already-built workspace with the CI image's GUI tools available:

```sh
. install/setup.bash
xvfb-run -a python3 src/decanstructor2/test/smoke_monitor.py smoke
```

The smoke publishes classic CAN and two full-length FD messages, operates the
sidebar controls, captures default/narrow visible, hidden and restored states,
and checks clean Escape and SIGINT exits. Inspect its PNGs: successful clicks,
matching screenshot dimensions and a message-ID count do not by themselves prove
that the correct rows, checkbox states or metadata were rendered. In CI, the
captures and application logs are under `smoke/` in each
`dc2-<distro>-results` artifact.

When changing the layout, inspect both sizes for readable MESSAGE IDS entries,
unobstructed metadata, usable all-hidden recovery, and complete FD matrices.
Update `docs/screenshots/monitor.png` with the actual default-size application
capture used by the README; keep its caption truthful, not a generated mockup.

### Node and GUI ownership

`DC2RosNode` handles ROS parameters, subscriptions and frame ingestion; its
constructor must return without opening a window or running the GUI loop.
`src/main.cpp` owns the cooperative single-threaded executor/window loop and
application shutdown. The headless `node_test` regression protects construction
and destruction independently of the GUI.

For documentation-only changes, validate links, anchors, terminology and any
edited shell examples. Reuse unchanged runtime evidence rather than rebuilding
the ROS matrix locally solely because prose changed.
