# physicsc-kahoot/arduino
This part of the codebase is flashed
onto the Arduino that will be used as
the controller.

This part is also licensed under the GNU General
Public License version 3 or any later version,
except the parts that aren't:
* `src/kahoot2/resetdbg.cpp`: see `src/kahoot2/resetdbg_LICENSE`; modified GPLv2

## Building & uploading
You need PlatformIO to build this codebase. The current configuration will build the correct
set of source files (`kahoot2` is the final project code.)