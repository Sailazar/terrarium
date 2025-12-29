3D Grid Module Editor
A 3D modeling tool built with C++ and Raylib. Create modular 3D structures with nodes, walls, and textures.
🚀 Quick Start
Compile & Run
Windows:
bashg++ test.cpp -o test.exe -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm
./test.exe
Linux:
bashg++ test.cpp -o test -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./test
🎮 Controls
Modes
KeyMode1Select - Pick nodes, create walls2Move Vertex - Drag nodes3Move Module - Move whole module4Add Node - Place new nodes5Connect - Link nodes
Camera

TAB - Toggle FPS/Cursor mode
RMB - Rotate camera
WASD - Move (FPS mode)

Actions

LMB - Select/drag (depends on mode)
SPACE - Create wall (3+ nodes in Select mode)
T - Apply texture to hovered wall
DELETE - Delete hovered item
N - Add new module
CTRL+D/C - Clone active module
CTRL+Z - Undo
CTRL+S / F5 - Export OBJ
CTRL+O / F6 - Import OBJ
G - Toggle grid
C - Toggle connections
F11 - Maximize window
ESC - Cancel (press twice to exit)

📁 Files

Place texture.png in folder for wall textures
model.obj - exported/imported model file

⚡ Tips

Use Select mode to create walls from 3+ nodes
Clone modules with Ctrl+D for quick duplication
Save often with Ctrl+S
Use arrow keys to move active module precisely


Built with Raylib