## Hardware & Software Requirements

### Supported Platforms

- **Linux** (x86_64 or ARM64)
    
    - Requires **gnome-terminal** to launch the clients
        
    
- **Windows** (x86_64 or ARM64)
    
    - Requires **cmd.exe** to launch the clients
        
    

### Build Tools & Languages

- **Git**
    
- **Python** ≥ 3.6 (for Conan 2.x)
    
- **Conan** 2.x
    
- **CMake** ≥ 3.28
    
- **C++23-capable compiler**
    
    - GCC ≥ 11
        
    - Clang ≥ 12
        
    - MSVC 2022 (v143)
        

### Dependencies (via Conan)

- cereal/1.3.2
    
- asio/1.32.0
    
- spdlog/1.14.1
    
- ftxui/6.0.2
    

These are pulled automatically by conan install.

### Optional (for docs)

- **Doxygen**
    
- **Graphviz**