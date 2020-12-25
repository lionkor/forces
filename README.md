# forces

A simple little physics project :)

A proper simulation of elastic collisions between circles / 2D-balls. 

## Build

1. `git clone` this repository **recursively**, so `git clone https://github.com/lionkor/forces --recursive`.
2. The following build instructions are linux-only. For Windows, you need to import the CMakeLists as a project and build it, then you probably have to help it with linking SFML.
3. run cmake, for example: `cmake -S . -B bin`
4. run make, for example (4 core multithreaded compilation): `make -C bin -j 4`
5. run `forces` binary in your build directory.
