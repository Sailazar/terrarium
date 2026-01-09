# Animated Planes - Save & Load Guide

## Overview
The application fully supports saving and loading animated planes with their textures. This guide explains how to use this feature.

## How to Save Animated Planes

### Step 1: Create Animated Planes
1. Drag and drop one or more PNG image files onto the application window
2. The application will create an animated plane with those images as frames
3. Multiple planes can be created by dropping different sets of images
4. You can also create an empty plane using the "New Plane" button in the Animation UI (press `A` to toggle)

### Step 2: Position Your Planes
- Click and drag planes in the 3D viewport to position them
- Use the Animation UI panel to adjust plane properties:
  - Size (width/height)
  - Frame timing
  - Billboard mode (always faces camera)
  - Play/pause animation

### Step 3: Save the Project
1. Press **CTRL + S** to open the save dialog
2. Enter a filename (without extension)
3. Click **SAVE** or press **ENTER**
4. Two files will be created:
   - `filename.obj` - The 3D geometry
   - `filename.dat` - Project data including animated planes

### What Gets Saved
The `.dat` file contains:
- **Texture Library**: All textures used in the project
- **Wall Textures**: Textures applied to walls
- **Animated Planes**: Complete plane data including:
  - Position in 3D space
  - Size (width and height)
  - All texture frames (saved as file paths)
  - Frame timing settings
  - Current frame
  - Play state (playing/paused)
  - Billboard mode setting

## How to Load Animated Planes

### Step 1: Load the Project
1. Press **CTRL + L** to open the load dialog
2. Enter the filename (without extension)
3. Click **LOAD** or press **ENTER**

### Step 2: Verify Loading
- The animated planes will be restored at their saved positions
- All textures will be reloaded from their original file paths
- The Animation UI will show all loaded planes (press `A` if not visible)
- Check the console output for loading status

### Important Notes About Loading
- **Texture files must exist**: The textures are loaded from their original file paths
- If a texture file is missing, that frame will fail to load
- The console will show which textures loaded successfully
- Planes will still be created even if some frames fail to load

## Debugging Save/Load Issues

### Console Output
When saving, you'll see:
```
=== Saving Project: filename.dat ===
Saving X animated planes...
  Plane 0: Position(x, y, z), N frames
    Frame 0: path/to/texture.png
    Frame 1: path/to/texture2.png
```

When loading, you'll see:
```
=== Loading Project: filename.dat ===
Loading X textures...
Loaded animated plane 0 with N frames
Total animated planes loaded: X
```

### Common Issues

**Issue: Save file shows 0 planes**
- Solution: Make sure you've created planes before saving
- Check that planes are visible in the Animation UI (press `A`)

**Issue: Planes don't appear after loading**
- Solution: Check console for texture loading errors
- Verify texture files still exist at their original paths
- Try pressing `A` to open Animation UI and verify planes are listed

**Issue: Textures are missing/black after loading**
- Solution: The texture files may have been moved or deleted
- Textures are loaded from absolute file paths
- Make sure texture files remain in the same location

**Issue: Planes appear but textures are wrong**
- Solution: Check that texture files weren't modified between save and load
- Verify the correct texture files exist at the saved paths

## Tips and Best Practices

### Organizing Textures
- Keep texture files in a dedicated folder
- Don't move or rename texture files after creating planes
- Consider using relative paths by keeping textures in the project folder

### Multiple Planes
- Each plane is saved independently
- You can have as many planes as needed
- Use descriptive filenames for textures to track what's loaded

### Animation Settings
- Frame timing and play state are preserved
- Billboard mode is saved and restored
- Current frame position is saved

### Workflow Recommendation
1. Create a project folder for your work
2. Store all texture images in that folder
3. Create your animated planes
4. Position and configure them
5. Save the project (CTRL + S)
6. Both `.obj` and `.dat` files will be in the same folder
7. When loading (CTRL + L), both files must exist

## Technical Details

### Save Format
The `.dat` file is a text file with this structure:
```
# Project Save File
TEXTURE_LIBRARY_START
<number of textures>
<texture path 1>
<texture path 2>
...
TEXTURE_LIBRARY_END

WALL_TEXTURES_START
...
WALL_TEXTURES_END

ANIMATED_PLANES_START
<number of planes>
PLANE 0
POSITION x y z
SIZE width height
FRAMETIME time
PLAYING 0/1
BILLBOARD 0/1
FRAMES <count>
<frame path 1>
<frame path 2>
...
ANIMATED_PLANES_END
```

### Texture Paths
- Absolute paths are used (e.g., `C:\Users\...\texture.png`)
- Paths are stored exactly as provided when files are dropped
- On load, `FileExists()` checks if files are still available

## Keyboard Shortcuts Quick Reference

- **CTRL + S**: Save project
- **CTRL + L**: Load project
- **A**: Toggle Animation UI panel
- **Click + Drag**: Move selected plane
- **ESC**: Deselect plane / Exit dialogs

## Status Messages

After saving, you'll see: `"Saved to <filename>"`
After loading, you'll see: `"Loaded <filename>"`

These messages appear briefly in the top-left corner of the viewport.