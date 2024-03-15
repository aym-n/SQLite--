# Building and Running the Project

## Prerequisites

- **CMake:** Ensure you have CMake (version 3.20 or later) installed on your system.
- **C++ Compiler:** A compatible C++ compiler (e.g., GCC, Clang) is also required.

## Building the Project

1. **Create a build directory:**
   ```bash
   mkdir build
   ```

2. **Configure the project:**
   ```bash
   cd build
   cmake -s .. -b .
   ```
   - This generates the build configuration files for your system.

3. **Build the executable and tests:**
   ```bash
   cmake --build .
   ```

## Running the Project

- **Run the executable:**
   ```bash
   ./build/main
   ```

## Running Tests

- **Run the test executable directly:**
   ```bash
   ./build/replTests
   ```

- **Or, use CTest for a more comprehensive test report:**
   ```bash
   cd build
   ctest
   ```

