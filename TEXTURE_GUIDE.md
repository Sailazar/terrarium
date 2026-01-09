# Texture Import Guide - Raylib 3D Grid Project

## 🎨 How to Import Textures

This guide explains how to add textures to your walls in the 3D Grid Editor.

---

## Quick Start

### Method 1: Simple Texture Loading (Recommended)

1. **Create a wall** (Select 3+ nodes in SELECT mode, press SPACE)
2. **Hover over the wall** with your mouse
3. **Place a texture file** in the project root directory with one of these names:
   - `texture.png`
   - `texture.jpg`
   - `wall.png`
   - `wall.jpg`
   - `tex.png`
   - `tex.jpg`
4. **Press `T` key** while hovering over the wall
5. **Done!** The texture is now applied

---

## Supported Texture Formats

Raylib supports the following image formats:
- **PNG** (.png) - Recommended, supports transparency
- **JPG/JPEG** (.jpg, .jpeg) - Good for photos
- **BMP** (.bmp) - Basic bitmap
- **TGA** (.tga) - Targa format
- **GIF** (.gif) - Animated (first frame only)
- **PSD** (.psd) - Photoshop (flattened)
- **HDR** (.hdr) - High Dynamic Range

---

## Texture File Names

The system automatically searches for texture files in this order:

1. `texture.png`
2. `texture.jpg`
3. `wall.png`
4. `wall.jpg`
5. `tex.png`
6. `tex.jpg`

**Tip:** Place your texture file in the project root with one of these names for automatic detection.

---

## Step-by-Step Tutorial

### Step 1: Create a Wall

```
1. Press `1` to enter SELECT MODE
2. Click to select 3 or more nodes (they should be coplanar)
3. Selected nodes will turn yellow
4. Press `SPACE` to create a wall
5. The wall appears as a semi-transparent blue surface
```

### Step 2: Prepare Your Texture

```
1. Find or create an image file (PNG or JPG recommended)
2. Recommended texture size: 512x512, 1024x1024, or 2048x2048 pixels
3. Rename it to "texture.png" or "wall.png"
4. Copy it to: C:\Users\Kris\raylib_project\
```

### Step 3: Apply the Texture

```
1. Move your mouse over the wall you want to texture
2. The wall should highlight when hovered
3. Press the `T` key
4. The texture is automatically loaded and applied!
```

### Step 4: Verify

```
- The wall should now display your texture
- Check the console output for confirmation messages
- If no texture file is found, a default blue texture is created
```

---

## Advanced Usage

### Multiple Textures

To use different textures on different walls:

**Option A: Rename and Apply**
1. Apply first texture to Wall 1
2. Rename/replace the texture file
3. Apply to Wall 2
4. Repeat as needed

**Option B: Modify the Code**
- Edit `test.cpp` line ~1296
- Add custom texture paths to the `texturePaths[]` array
- Example: Add `"brick.png"`, `"metal.jpg"`, etc.

### Custom Texture Paths

Current texture search paths (in code):
```cpp
const char* texturePaths[] = {
    "texture.png", 
    "texture.jpg", 
    "wall.png", 
    "wall.jpg", 
    "tex.png", 
    "tex.jpg"
};
```

To add more paths, edit this array in `test.cpp` at line ~1296.

### Texture Coordinates

The system automatically generates UV coordinates based on:
- Wall vertex positions (X, Y, Z)
- Bounding box of the wall
- Largest dimension is used for mapping

**Automatic UV Mapping:**
- If X-range is largest → XY mapping
- If Y-range is largest → YZ mapping  
- If Z-range is largest → XZ mapping

---

## Texture Recommendations

### Size
- **Minimum:** 256x256 pixels
- **Recommended:** 512x512 or 1024x1024 pixels
- **Maximum:** 4096x4096 pixels (depending on GPU)
- **Best Practice:** Use power-of-2 dimensions (256, 512, 1024, 2048, 4096)

### Format
- **Best:** PNG (lossless, supports transparency)
- **Photos:** JPG (smaller file size)
- **Avoid:** GIF (limited colors)

### Content
- **Seamless Textures:** Use tileable textures for best results
- **Resolution:** Higher resolution = better quality but slower performance
- **Compression:** Keep file sizes reasonable (< 5MB per texture)

---

## Troubleshooting

### Problem: "No texture file found"

**Solution:**
1. Check that texture file is in the project root: `C:\Users\Kris\raylib_project\`
2. Verify filename matches one of the supported names
3. Check file extension (must be .png, .jpg, etc.)
4. Ensure file is not corrupted (try opening it in an image viewer)

### Problem: Texture appears stretched or distorted

**Cause:** Wall geometry is non-rectangular or complex

**Solution:**
1. Use simpler wall shapes (rectangles work best)
2. Keep walls planar (all nodes in same plane)
3. For complex shapes, use multiple smaller walls

### Problem: Texture is upside down or rotated

**Solution:**
- Rotate the texture in an image editor before importing
- The UV mapping is automatic and based on wall orientation

### Problem: Texture loading is slow

**Solution:**
1. Reduce texture resolution (e.g., 1024x1024 instead of 4096x4096)
2. Convert to JPG if using PNG
3. Optimize image file size

### Problem: "Failed to load texture from file"

**Solutions:**
1. **File permissions:** Ensure the file is not read-protected
2. **File corruption:** Re-save the image
3. **Format issues:** Convert to PNG and try again
4. **Path issues:** Make sure file is in correct directory

---

## Console Output

When loading textures, check the console for messages:

**Success:**
```
Attempting to load texture on wall 0 in module 0
Found texture file: texture.png
Successfully loaded texture: texture.png
```

**File Not Found:**
```
Attempting to load texture on wall 0 in module 0
No texture file found, creating default blue texture
Created default blue texture for wall
```

**Error:**
```
Attempting to load texture on wall 0 in module 0
Failed to load texture from file: texture.png
```

---

## Keyboard Controls

| Key | Action |
|-----|--------|
| `T` | Load/Apply texture to hovered wall |
| `1` | SELECT MODE (required to create walls) |
| `SPACE` | Create wall from selected nodes |
| `DEL` | Delete selected nodes/walls |

---

## Texture Workflow Example

### Creating a Textured Room

```
Step 1: Create the floor
  - Select 4 corner nodes
  - Press SPACE
  - Press T (applies default or file texture)

Step 2: Create walls
  - Select 4 nodes for each wall
  - Press SPACE for each wall
  - Hover over each wall and press T

Step 3: Add custom textures
  - Copy floor.png to project folder, rename to texture.png
  - Hover floor, press T
  - Replace texture.png with wall_brick.png
  - Hover wall, press T
  - Repeat for each wall
```

---

## Tips & Best Practices

### Performance
- ✅ Use compressed JPG for large textures
- ✅ Keep texture count reasonable (< 50 unique textures)
- ✅ Reuse textures when possible
- ❌ Don't use 4K textures unless necessary

### Visual Quality
- ✅ Use seamless/tileable textures
- ✅ Match texture resolution to wall size
- ✅ Use consistent texture style across project
- ❌ Don't mix photo-realistic with cartoon textures

### Organization
- ✅ Create an `assets/` or `textures/` folder for multiple textures
- ✅ Name textures descriptively (brick_red.png, metal_rusty.jpg)
- ✅ Keep backup copies of original textures
- ❌ Don't clutter project root with many texture files

---

## Free Texture Resources

### Websites
- **Poly Haven** (polyhaven.com) - Free CC0 textures
- **Textures.com** - Free + paid textures
- **OpenGameArt.org** - Free game textures
- **CC0 Textures** (cc0textures.com) - Public domain

### Texture Types to Look For
- Brick walls
- Concrete
- Wood planks
- Metal plates
- Stone tiles
- Fabric
- Grass/Terrain

---

## Default Texture Behavior

If no texture file is found, the system automatically:
1. Creates a 256x256 pixel solid blue texture
2. Applies it to the wall
3. Stores it in memory

**To replace the default:**
- Simply place a texture file and press T again
- The old texture is automatically unloaded

---

## Technical Details

### Texture Storage
- Textures are stored per-wall in the `Wall` struct
- Each wall has `Texture2D texture` and `bool hasTexture`
- Textures are GPU-resident (VRAM)

### Memory Management
- Old textures are automatically unloaded when replaced
- Textures are unloaded when walls are deleted
- Proper cleanup on application exit

### Rendering
- Walls with textures use `DrawMesh()` with material
- Non-textured walls use simple triangle rendering
- Both front and back faces are rendered

---

## Code Reference

**Texture Loading:** `test.cpp` lines 1290-1335  
**Wall Creation:** `test.cpp` lines 813-850  
**Wall Rendering:** `test.cpp` lines 234-407  

---

## FAQ

**Q: Can I use animated textures?**  
A: Not directly. Only the first frame of GIFs is used. For animation, you'd need to manually update textures per frame.

**Q: Can I have transparency in textures?**  
A: Yes! Use PNG format with alpha channel. The renderer supports transparency.

**Q: How many textures can I load?**  
A: Limited by GPU memory. Typically 100+ textures of 1024x1024 should work fine.

**Q: Can I export textured walls to OBJ?**  
A: The current OBJ export doesn't include texture coordinates. This is a planned feature.

**Q: Why does my texture look blurry?**  
A: Increase texture resolution or reduce wall size. Raylib uses bilinear filtering by default.

**Q: Can I change texture filtering?**  
A: Yes, after loading, call `SetTextureFilter(texture, TEXTURE_FILTER_POINT)` for pixel-art style.

---

## Future Enhancements

Planned features:
- [ ] Drag-and-drop texture loading
- [ ] Texture browser/selector UI
- [ ] Multiple texture slots per wall
- [ ] Texture scaling/rotation controls
- [ ] Normal maps and PBR materials
- [ ] OBJ export with UV coordinates
- [ ] Texture atlas support

---

**Last Updated:** January 7, 2025  
**Version:** 1.0  
**Compatible With:** Raylib 3D Grid Project v1.0+

---

**Quick Reference Card:**
```
┌──────────────────────────────────────┐
│  TEXTURE QUICK REFERENCE             │
├──────────────────────────────────────┤
│  1. Copy texture.png to project root │
│  2. Create wall (Select + SPACE)     │
│  3. Hover over wall                  │
│  4. Press T                          │
│  5. Done!                            │
└──────────────────────────────────────┘
```
