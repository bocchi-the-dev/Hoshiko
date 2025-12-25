![Hoshiko Banner](https://github.com/bocchi-the-dev/banners/blob/main/hoshiko.banner.png?raw=true)

# Hoshiko

**Hoshiko** is an application designed to **block ads** in specific Android apps.
It acts as a **user interface add-on** for [Re-Malwack](https://github.com/ZG089/Re-Malwack), providing a more convenient way to control app-level ad blocking.

---

## Overview

* **hoshiko-cli** serves as the foundation of the project.
  The Android app depends on this code, meaning **the CLI version will never be deprecated**.
* **hoshiko-app** provides an Android-friendly UI built around the CLI logic.

---

## Building the Project

### Prerequisites

Before building, make sure you have the following installed:

* [Android Studio](https://developer.android.com/studio)
* [Gradle](https://gradle.org/)
* [Android NDK r27d](https://developer.android.com/ndk/downloads)
* GNU/Linux or [WSL](https://learn.microsoft.com/en-us/windows/wsl/)

---

### Building the Android App

To build the Android application release:

```bash
cd hoshiko-app/src
./gradlew assembleRelease
```

This will generate a signed release APK in:

```
app/build/outputs/apk/release/
```

---

### Building the CLI version

Before the first build, set the `CC_ROOT` variable to your NDK root path inside the `make.sh`:

```makefile
CC_ROOT=/path/to/android-ndk
```
#### Build Arguments
| Argument | Description                                              |
| -------- | -------------------------------------------------------- |
| `SDK`    | Minimum supported Android SDK version (e.g. `28`)        |
| `ARCH`   | Target architecture — `arm64` or `arm`                   |
> **NOTE**: We don't support `x86` and `x86_64` architectures.

#### Build targets
| Target   | Description                                              |
| -------- | -------------------------------------------------------- |
| `yuki`   | Builds the hoshiko daemon. |
| `alya`   | Builds the hoshiko daemon manager. |
| `all`    | Builds yuki and alya together. |
| `help`   | Shows the available build targets. |
| `clean`  | Removes the previous built remnants. |

Then build using:

```bash
./make.sh SDK=28 ARCH=arm64 <target>
```

---

## CLI Tools

### Alya

`Alya` is the **command-line manager** for controlling Hoshiko’s background daemon (`yuki`).

#### Usage Examples

```bash
alya -a | --add-app <package>                   # Add an app to the blocklist
alya -r | --remove-app <package>                # Remove an app from the blocklist
alya -e | --export-package-list <path>          # Export the package list
alya -i | --import-package-list <path>          # Import a package list
alya -x | --enable-daemon                       # Enable the Yuki daemon
alya -d | --disable-daemon                      # Disable the Yuki daemon
alya -k | --kill-daemon                         # Kill the Yuki daemon
alya -l | --lana-app                            # Redirect output to log file instead of stdout
```

---

### Yuki

`Yuki` is Hoshiko’s **background daemon** that monitors and enforces ad-blocking policies.

> **Note:**
> `Yuki` is designed to run as a background service (e.g., via `init`).
> Running it inside **Termux** is not recommended as it always returns `NULL` on current package request calls. There is a reason to it.

---

## Licensing

### Application License

**Hoshiko** is licensed under the [GNU General Public License v3.0 (GPLv3)](./LICENSE).
You are free to use, modify, and distribute this software under the same license terms.

### Typeface License

The **Montserrat** typeface is licensed under the
**[SIL Open Font License, Version 1.1 (OFL-1.1)](https://openfontlicense.org/)**,
which allows free use, modification, and redistribution — provided that the font is not sold on its own and that modified versions are renamed.

---

## Notes

* Please build the cli version and put the built binaries inside `/data/adb/Re-Malwack/` so that the application can access it. Thank you!