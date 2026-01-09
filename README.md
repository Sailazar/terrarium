# Raylib 3D Grid Project

A 3D grid-based modeling tool built with Raylib that allows you to create, manipulate, and export 3D structures.

## Project Structure

```
raylib_project/
├── include/          # Raylib header files (raylib.h, raymath.h, rlgl.h)
├── lib/              # Raylib library files (libraylib.a)
├── .vscode/          # VS Code configuration
│   ├── c_cpp_properties.json
│   ├── tasks.json
│   └── settings.json
├── test.cpp          # Main source file
├── test.exe          # Compiled executable
└── compile_commands.json
```

## Features

- Create and manipulate 3D nodes in a grid structure
- Multiple modules support
- Connect nodes within and across modules
- **Wall creation with texture support**
- OBJ file import/export
- Multiple editing modes:
  - Select mode
  - Move vertex mode
  - Move module mode
  - Add node mode
  - Connect nodes mode
  - Rotate module mode
  - Scale mode

### Texture System
- Apply custom textures to walls with a single keypress
- Supports PNG, JPG, BMP, TGA, GIF, PSD formats
- Automatic UV coordinate generation
- Memory-managed texture loading/unloading
- Default texture fallback if no file found

## Building the Project

### Prerequisites

- **w64devkit** (or any MinGW-w64 toolchain with g++)
- Raylib library (already included in `lib/` and `include/`)

### Compile Command

```bash
g++ test.cpp -o test.exe -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

### VS Code Build Task

Press `Ctrl+Shift+B` to build using the configured task, or use the Terminal menu → Run Build Task.

## Running the Application

Simply execute the compiled binary:

```bash
./test.exe
```

Or double-click `test.exe` in File Explorer.

## Controls

### Mode Switching
- `1` - SELECT MODE (select nodes, create walls)
- `2` - MOVE VERTEX MODE (move individual nodes)
- `3` - MOVE MODULE MODE (move entire modules)
- `4` - ADD NODE MODE (create new nodes)
- `5` - CONNECT MODE (connect nodes)
- `6` - ROTATE MODULE MODE (rotate modules)
- `7` - SCALE MODE (scale node positions/geometry)
- `8` - SCALE SPHERE MODE (scale visual sphere size only)

### Actions
- `SPACE` - Create wall from selected nodes (SELECT mode)
- `T` - Apply texture to hovered wall
- `N` - Add new module
- `DEL` - Delete selected nodes/walls/modules
- `TAB` - Toggle cursor/FPS camera
- `G` - Toggle grid display
- `C` - Toggle connection lines
- `ESC` - Cancel operation (press twice to exit)

### Camera
- `RMB` - Rotate camera
- `Arrow Keys` - Move/rotate (mode-dependent)
- Mouse Wheel - Zoom/adjust distance

### File Operations
- `CTRL+S` or `F5` - Export to OBJ
- `CTRL+O` or `F6` - Import from OBJ
- `CTRL+D` or `CTRL+C` - Clone selected
- `CTRL+Z` - Undo
- Drag & Drop - Import model.obj file

### Textures
1. Create a wall (Select 3+ nodes, press SPACE)
2. Place texture file in project root (texture.png, wall.png, etc.)
3. Hover over wall with mouse
4. Press `T` to apply texture

See `TEXTURE_GUIDE.md` for detailed texture instructions.

## Troubleshooting

### IntelliSense Errors in VS Code

If you see errors like `'raylib.h' file not found` in VS Code but the project compiles successfully:

1. **Reload the VS Code Window**:
   - Press `Ctrl+Shift+P`
   - Type "Developer: Reload Window" and press Enter

2. **Reset IntelliSense Database**:
   - Press `Ctrl+Shift+P`
   - Type "C/C++: Reset IntelliSense Database" and press Enter

3. **Restart the C++ Extension**:
   - Press `Ctrl+Shift+P`
   - Type "C/C++: Restart IntelliSense for Active File"

4. **Check Configuration**:
   - Ensure `.vscode/c_cpp_properties.json` exists
   - Verify the include paths are correct
   - Make sure `compile_commands.json` is present in the root directory

### Compilation Errors

1. **Compiler not found**: Make sure w64devkit (or MinGW-w64) is installed and in your PATH
2. **Linker errors**: Verify that `libraylib.a` exists in the `lib/` directory
3. **Header not found**: Check that all header files are in the `include/` directory

### Runtime Errors

- **Missing DLL**: If you get DLL errors, ensure all required libraries are in your PATH or the same directory as the executable
- **Graphics errors**: Update your graphics drivers

## Configuration Files

### .vscode/c_cpp_properties.json

Configures IntelliSense with:
- Include paths for Raylib headers
- Compiler path (w64devkit)
- C++17 standard
- Windows GCC IntelliSense mode

### .vscode/tasks.json

Build task configuration with proper compiler flags and library linking.

### .vscode/settings.json

Additional C++ extension settings for better IntelliSense behavior.

### compile_commands.json

Compilation database for improved IntelliSense accuracy.

## Export/Import

The application supports OBJ file format:
- **Export**: Save your 3D models as `.obj` files
- **Import**: Load existing `.obj` files into the editor

## Notes

- The project uses C++17 standard
- Built and tested on Windows with w64devkit
- Raylib version included in the `lib/` directory
- The executable (`test.exe`) and object files (`.obj`) are already compiled

## Documentation Files

- **README.md** - This file (project overview)
- **TEXTURE_GUIDE.md** - Comprehensive texture import guide
- **TEXTURE_QUICK_REF.txt** - Quick reference for texture system
- **COLOR_SCHEME.md** - UI color palette documentation
- **QUICK_START.md** - Fast-start guide
- **FIXES_APPLIED.md** - Technical configuration details
- **fix_intellisense.bat** - Automated IntelliSense fix script

## Color Scheme

The UI uses a tactical surveillance-inspired color palette:
- **CYBER_CYAN** (#00B4D8) - Primary selections and operations
- **TACTICAL_ORANGE** (#FF9500) - Action modes and modifications
- **AMBER_ALERT** (#FFBF00) - Stats, alerts, and important info
- Dark cyan variations for text hierarchy

See `COLOR_SCHEME.md` for complete palette details.

## License

(Add your license information here)

## Author

(Add your information here)