# stinger

**stinger** is a comprehensive operating system and shell environment project. it features a rich set of command-line utilities, core system components, and development tools. inspired by traditional unix-like systems, stinger aims to provide a robust and versatile platform with a unique, modern flavor.

---

### features

* **extensive command-line utilities:** a vast collection of tools for file manipulation, system information, networking, and text processing. 
    * *standard:* `ls`, `grep`, `cp`, `rm`, `ps`, `top`, `ping`, `ssh`, `wget`.
    * *custom/playful:* `boykisser`, `rickroll`, `shrug`, `yeet`, `doge`, `ligma`,  and `yes`.
* **bootloader and kernel:** includes integrated bootloader components (`boot/`) and a dedicated kernel (`kernel/`, `build/kernel.elf`) for system startup and core operations.
* **device drivers:** broad hardware support managed through a dedicated drivers module (`drivers/`).
* **authentication system:** built-in `auth/` module for secure user management and identity verification.
* **networking stack:** comprehensive networking capabilities (`net/`, `fetch/`) featuring full dhcp client support.
* **file system:** robust implementation of file system primitives (`fs/`).
* **graphical user interface:** modular components for a dedicated graphical interface (`gui/`).
* **build system:** utilizes `makefile` for compilation, supported by python scripts for utility tasks. node.js integration (`package.json`) is available for potential frontend or build tooling.
* **release artifacts:** available in multiple formats, including iso, zip, and rar archives.

---

### getting started

to build and run stinger, follow these steps:

1.  **dependencies:** ensure you have the necessary build tools installed (e.g., a c compiler, make, and python).
2.  **navigation:** open your terminal and navigate to the project root directory.
3.  **compilation:** execute the build command. this is typically `make` (if using the default makefile configuration) or a specific execution script like `python run.bat`.

> **note:** once the build is successful, the compiled kernel and utilities can be found in the `build/` directory. pre-built versions are available in the release archives.

---

### license

this project is licensed under the mit license. it is provided **as-is**, without any warranty of any kind.

