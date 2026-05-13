# POP Raylib starter

Simple starter application for POP C assignment.

# Building

## 1. Using the Raylib Installer (Windows)

If you installed Raylib using the official [Raylib Installer for Windows](https://github.com/raysan5/raylib/releases), you can easily compile and run the game using the provided environment:

1. Open `src/main.c` in the pre-configured **Notepad++** that came with the installer.
2. Press `F6` to open the execute dialog.
3. Select `raylib_compile_execute` and click **OK**.
4. The game should compile and open immediately.

## 2. Using Standalone Compilers (No Raylib Launcher)

If you prefer to compile the game manually without the Raylib installer, you will need a C compiler and the Raylib library.

### Required Tools & Resources:
- **Windows**: [MinGW-w64 (GCC)](https://www.msys2.org/) or Microsoft Visual C++ (via [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)).
- **macOS**: Clang (install via Terminal: `xcode-select --install`).
- **Linux (Debian/Ubuntu)**: GCC (install via Terminal: `sudo apt install build-essential libraylib-dev`).

### Manual Compilation Example (GCC / MinGW):
From the root of the project, run:
```bash
gcc src/main.c -o game.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -lraylib -lopengl32 -lgdi32 -lwinmm
```
*(Adjust the `-I` paths and provide `-L` linker flags if your Raylib libraries are installed elsewhere).*

Then, to run the compiled game, simply execute:
```bash
./game.exe
```

## 3. WebAssembly (WASM) Build

To build for web/WASM, run the command:

```bash
/opt/pop/bin/build-wasm.sh src/main.c
```

This will generate a directory *out* with the WASM and index.html files for the 
Raylib program.

# Running (WASM)

The very first time you run a POP WASM application you must run the command:

```bash
/opt/pop/bin/allocate_port.sh
```

You might need to start a new terminal instance for the update to take effect.
To check that everything is fine run the command:

```bash
echo $MY_PORT
```

This should output a 5 digit number.


To run the Raylib program in *out* simply run the command:

```bash
/opt/pop/bin/run-wasm.sh
```

This will run a web server that serves the *out* on the port you allocated above. This is forwarded from the 
remote server to your local machine, which means you can simply open the corresponding web page within a browser 
on your local machine using the address:

```bash
localhost:XXXXX
```

where *XXXXX* is the port number you allocated above.