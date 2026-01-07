# Quick Start Guide - Raylib 3D Grid Project

## ⚡ TL;DR - Fix IntelliSense Errors NOW

If you're seeing red error squiggles in VS Code but the code compiles:

1. **Close VS Code completely**
2. **Run** `fix_intellisense.bat`
3. **Open VS Code**
4. **Press** `Ctrl+Shift+P`
5. **Type** "C/C++: Reset IntelliSense Database" → Enter
6. **Press** `Ctrl+Shift+P` again
7. **Type** "Developer: Reload Window" → Enter
8. **Wait 30 seconds** for IntelliSense to rebuild

✅ Done! Errors should be gone.

---

## 🚀 First Time Setup

### Step 1: Verify Requirements
- ✅ **w64devkit** installed at `C:\w64devkit\`
- ✅ **VS Code** with C/C++ extension installed
- ✅ **Raylib files** in `include/` and `lib/`

### Step 2: Open Project
```bash
cd C:\Users\Kris\raylib_project
code .
```

### Step 3: Build
- Press `Ctrl+Shift+B` (Build)
- Or run: `g++ test.cpp -o test.exe -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm`

### Step 4: Run
- Execute `test.exe`
- Or double-click in File Explorer

---

## 🔧 Build Commands

### Standard Build
```bash
g++ test.cpp -o test.exe -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

### With Debug Symbols
```bash
g++ test.cpp -o test.exe -g -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

### With Optimization
```bash
g++ test.cpp -o test.exe -O2 -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

---

## 🐛 Troubleshooting

### Problem: "raylib.h not found" (Red Squiggles)
**Solution:**
1. Close VS Code
2. Run `fix_intellisense.bat`
3. Reopen VS Code
4. `Ctrl+Shift+P` → "C/C++: Reset IntelliSense Database"
5. `Ctrl+Shift+P` → "Developer: Reload Window"

### Problem: Build Task Not Working
**Solution:**
- Ensure `g++` is in PATH, or
- Use full path: `C:\w64devkit\bin\g++.exe`
- Check `.vscode\tasks.json` exists

### Problem: Code Compiles But Has Errors in Editor
**Cause:** IntelliSense cache issue (not a real error!)

**Solution:**
```
Ctrl+Shift+P → "C/C++: Restart IntelliSense for Active File"
```

### Problem: Missing DLL at Runtime
**Solution:**
- Add `C:\w64devkit\bin` to system PATH
- Or copy required DLLs to project folder

---

## 📁 Project Files Overview

```
raylib_project/
├── test.cpp                    # Main source code
├── test.exe                    # Compiled executable
├── include/
│   ├── raylib.h               # Raylib header
│   ├── raymath.h              # Math utilities
│   └── rlgl.h                 # OpenGL abstraction
├── lib/
│   └── libraylib.a            # Static library
├── .vscode/
│   ├── c_cpp_properties.json  # IntelliSense config
│   ├── tasks.json             # Build tasks
│   └── settings.json          # VS Code settings
├── compile_commands.json       # Compilation database
├── fix_intellisense.bat       # Auto-fix script
├── README.md                  # Full documentation
├── FIXES_APPLIED.md           # Technical details
└── QUICK_START.md             # This file
```

---

## 🎮 Application Features

- **3D Grid Editor** - Create and manipulate 3D structures
- **Multiple Modules** - Organize nodes into separate modules
- **Node Connections** - Connect nodes within/across modules
- **Wall Creation** - Build walls with texture support
- **OBJ Export/Import** - Save and load 3D models
- **Multiple Edit Modes**:
  - Select, Move Vertex, Move Module
  - Add Node, Connect Nodes
  - Rotate Module, Scale

---

## ⌨️ Keyboard Shortcuts (VS Code)

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+B` | Build project |
| `Ctrl+Shift+P` | Command palette |
| `F5` | Debug (if configured) |
| `Ctrl+` ` | Open terminal |

---

## 📚 Additional Resources

- **Full Documentation:** `README.md`
- **Technical Fixes:** `FIXES_APPLIED.md`
- **Raylib Docs:** https://www.raylib.com/
- **w64devkit:** https://github.com/skeeto/w64devkit

---

## 💡 Tips

1. **Always close VS Code** before running `fix_intellisense.bat`
2. **Compilation success** is what matters - IntelliSense errors are often false positives
3. **Existing `test.exe`** proves your code compiles correctly
4. **Reload Window** is faster than restarting VS Code
5. **IntelliSense takes time** - wait 30-60 seconds after reload

---

## ✅ Everything Working Checklist

After setup, verify:
- [ ] No red squiggles under `#include "raylib.h"`
- [ ] `Vector3`, `Texture2D` types recognized
- [ ] Code completion shows Raylib functions
- [ ] `Ctrl+Shift+B` builds successfully
- [ ] `test.exe` runs without errors
- [ ] No errors in "Problems" panel

---

## 🆘 Still Having Issues?

1. Check `FIXES_APPLIED.md` for detailed troubleshooting
2. Verify w64devkit is installed: `C:\w64devkit\bin\g++.exe --version`
3. Reinstall C/C++ extension in VS Code
4. Check VS Code C++ extension output logs
5. Ensure all files in `include/` and `lib/` exist

---

**Last Updated:** January 7, 2025  
**Project Status:** ✅ All fixes applied, ready to use!