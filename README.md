
## Building and testing

- install SDK almost according to https://developer.repebble.com/sdk/
  - install node
  - `sudo apt install libsdl1.2debian libfdt1`
  - `brew install uv`
- create virtual env `conda create --prefix conda-env python=3.10` and activate it with `conda activate conda-env`
- `uv pip install pebble-tool`
- `pebble sdk install latest`
- `pebble build`
- `pebble install --emulator basalt`

Once it's set up, just run:

- `conda activate conda-env`
- `pebble build`
- `pebble install --emulator basalt`

If installing to the emulator fails:

- build and install the pebble-demo, or some other project taht you know works
- `pebble clean`
- `uv pip install --upgrade --reinstall pebble-tool`
- `rm -rf ~/.pebble-sdk`
- `pebble sdk install latest`
