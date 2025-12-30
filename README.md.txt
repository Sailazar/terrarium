# 3D Grid Module Editor

A powerful 3D modeling tool built with C++ and Raylib. Create modular 3D structures with nodes, walls, textures, and advanced transformation capabilities including cross-module connections.

## 🚀 Quick Start

### Compile & Run

**Windows:**
```bash
g++ test.cpp -o test.exe -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm
./test.exe
```

**Linux:**
```bash
g++ test.cpp -o test -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./test
```

## 🎮 Controls

### Modes

| Key | Mode | Description |
|-----|------|-------------|
| **1** | Select | Pick nodes, create walls, select across modules |
| **2** | Move Vertex | Drag individual nodes |
| **3** | Move Module | Move entire modules |
| **4** | Add Node | Place new nodes in 3D space |
| **5** | Connect | Create connections between nodes (even across modules!) |
| **6** | Rotate Module | Rotate selected module with precision |
| **7** | Scale | Adjust visual size of selected nodes |

### Camera Controls

| Key | Action |
|-----|--------|
| **TAB** | Toggle FPS/Cursor mode |
| **RMB** | Rotate camera (cursor mode) |
| **WASD** | Move camera (FPS mode) |
| **SPACE** | Move up (FPS mode) |
| **SHIFT** | Move down (FPS mode) |

### Selection & Actions

| Key | Action |
|-----|--------|
| **LMB** | Select/drag (mode dependent) |
| **Double-click** | Select all nodes in module (Select mode) |
| **Drag selection** | Box select multiple nodes |
| **SPACE** | Create wall from 3+ selected nodes |
| **DELETE** | Delete selected/hovered item |

### Advanced Features

| Key | Action |
|-----|--------|
| **T** | Apply texture to hovered wall |
| **N** | Add new module |
| **CTRL+D/C** | Clone selection or active module |
| **CTRL+Z / BACKSPACE** | Undo |
| **CTRL+S / F5** | Export to OBJ |
| **CTRL+O / F6** | Import from OBJ |

### Mode 6: Rotate Module

| Key | Action |
|-----|--------|
| **Arrow Keys** | Rotate in 5° increments |
| **PAGE UP/DOWN** | Roll rotation (Z-axis) |
| **LMB + Drag** | Free-form mouse rotation |
| **R + Arrows** | Quick 15° rotation (any mode) |

### Mode 7: Scale Nodes

| Key | Action |
|-----|--------|
| **+/-** | Scale sphere size up/down |
| **Mouse Wheel** | Smooth scaling |
| **LMB + Drag** | Interactive scaling (drag up/down) |

### View Options

| Key | Action |
|-----|--------|
| **G** | Toggle grid display |
| **C** | Toggle connection lines |
| **F11** | Maximize/restore window |

### Navigation

| Key | Action |
|-----|--------|
| **Arrow Keys** | Move active module (cursor mode) |
| **ESC** | Cancel operation (press twice within 2s to exit) |

## ✨ Features

### Core Capabilities
- **Modular Design** - Work with multiple independent modules
- **Cross-Module Connections** - Link nodes between different modules (shown in cyan)
- **Cross-Module Walls** - Create walls spanning multiple modules
- **Node Scaling** - Individual visual size adjustment per node
- **Texture Mapping** - Apply textures to walls with automatic UV mapping
- **Undo System** - Full history with CTRL+Z
- **OBJ Export/Import** - Save and load your models

### Advanced Tools
- **Drag Selection** - Box select multiple nodes at once
- **Smart Rotation** - Keyboard precision or smooth mouse rotation
- **Module Cloning** - Duplicate entire modules or just selected nodes
- **Visual Feedback** - Color-coded nodes and connections
  - Yellow: Selected nodes
  - Green: Hovered (Select mode)
  - Red: Hovered (Move mode)
  - Cyan: Cross-module connections
  - Gray: Local connections
  - Orange: Active module

## 📁 Files

- **texture.png** - Place in project folder for wall textures
  - Supported formats: `.png`, `.jpg`
  - Will auto-search: `texture.png`, `texture.jpg`, `wall.png`, `wall.jpg`, `tex.png`, `tex.jpg`
- **model.obj** - Exported/imported model file

## 💡 Tips & Workflow

### Creating Walls
1. Press **1** (Select mode)
2. Click to select 3+ nodes (can be from different modules!)
3. Press **SPACE** to create wall
4. Hover wall and press **T** to add texture

### Cross-Module Modeling
1. Create multiple modules with **N**
2. Use mode **5** (Connect) to link nodes between modules
3. Select nodes from different modules (they turn yellow)
4. Press **SPACE** to create walls that span modules

### Module Transformations
1. Click module in mode **3** to activate it
2. Use **Arrow keys** to move precisely
3. Switch to mode **6** and use arrows or mouse to rotate
4. Press **R + Arrows** for quick 15° rotations

### Node Scaling for Visual Hierarchy
1. Select important nodes in mode **1**
2. Switch to mode **7**
3. Use **+/-** or **Mouse Wheel** to adjust size
4. Create visual emphasis on key connection points

### Best Practices
- Save often with **CTRL+S**
- Use **CTRL+D** to clone and experiment
- Enable grid (**G**) for alignment
- Toggle connections (**C**) to reduce visual clutter
- Use **CTRL+Z** freely - full undo available

## 🎨 Color Guide

| Color | Meaning |
|-------|---------|
| **Dark Purple** | Default node |
| **Yellow** | Selected nodes |
| **Green** | Hovered (Select mode) |
| **Red** | Hovered (Move mode) |
| **Blue** | Module hovered |
| **Orange** | Active module |
| **Lime** | Connection start node |
| **Cyan** | Cross-module connections |
| **Gray** | Local module connections |

## 🔧 System Requirements

- OpenGL 3.3+ compatible GPU
- Raylib 5.0+
- C++11 or later compiler

## 📝 Technical Notes

- Supports both local connections (within module) and cross-module connections
- Walls are stored in modules with automatic node copying for cross-module walls
- Each node has individual scale property (0.1x to 5.0x)
- Full state preservation in undo system
- OBJ export includes vertices, lines (connections), and faces (walls)

---

**Built with ❤️ using Raylib**

Project supports complex 3D modeling workflows with modular architecture and intuitive controls.