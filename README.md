# MIC8-Interpreter

A multi-instance CHIP-8 interpreter written in C++.

Some inessential features are currently not implemented.

## Dependencies

You will need GLFW on your system. (http://www.glfw.org)

Linux:

```bash
apt-get install libglfw-dev
```

Mac OS X:

```bash
brew install glfw
```

MSYS2:

```bash
pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
```

## Installation

Clone the repository:

```bash
git clone --recurse-submodules https://github.com/AE7TB99/MIC8-Interpreter
```

Navigate to the project directory and build:

```bash
cd MIC8-Interpreter/
make
```
