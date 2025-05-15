## Instalation guide

**Clone a repo**
```sh
git clone git@gitlab.mff.cuni.cz:teaching/nprg041/2024-25/repetenti/yuriicho.git
cd yuriicho/project/src/
```

**Use `pip` to install Conan**:
```sh
pip install conan
```

**Create or Update the Default Profile**:
```sh
conan profile new default --detect
conan profile update settings.compiler.cppstd=23 default
```

**Dependencies are installed using the Conan command**:
```sh
# For Linux/Mac
conan install . \
    -g CMakeToolchain \
    -g CMakeDeps \
    --build=missing \
    -s:h build_type=Debug \
    -s:b build_type=Debug
# For Windows
conan install .  `
    -g CMakeToolchain `
    -g CMakeDeps `
    --build=missing `
    -s:h build_type=Debug `
    -s:b build_type=Debug
```
- This fetches the required libraries and ensures compatibility with the target platform.

**Generate Build Files (Configure)**:
```sh
# For Linux/Mac
cmake --preset conan-debug
# For Windows
cmake --preset conan-default
```

**Build the Project**:
```sh
cmake --build --preset conan-debug
```

**Enter the Build Output Directory**:
```sh
cd build/Debug/
```

**Run the Server Executable**:
```sh
# For Linux/Mac
./canasta_server 2
# or ./canasta_server 4
# For Windows
.\canasta_server.exe 2
# or .\canasta_server.exe 4
```

-----

### To build the docs you’ll need:

- [Doxygen](https://www.doxygen.nl/download.html)  
- [Graphviz](https://www.graphviz.org/download/)

**Once those are installed, run**:
```sh
cd ../docs
doxygen Doxyfile
```