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
