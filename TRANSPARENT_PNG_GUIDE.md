# Transparent PNG Support for Animated Planes

## Overview
The application now fully supports transparent PNG images for animated planes, allowing you to create sprites and overlays with alpha channel transparency.

## What's Supported

### PNG Transparency Features
✅ **Full Alpha Channel**: 32-bit RGBA PNG images with transparency
✅ **Partial Transparency**: Semi-transparent pixels (0-255 alpha values)
✅ **Double-Sided Rendering**: Planes visible from both front and back
✅ **Proper Blending**: Transparent pixels blend correctly with background
✅ **Billboard Mode**: Transparent PNGs work in billboard mode too

## How It Works

### Texture Loading
When you drag and drop a PNG file onto the application:
1. Raylib's `LoadTexture()` automatically detects PNG format
2. Alpha channel is preserved if present in the PNG
3. Texture is stored with full RGBA data
4. No additional configuration needed!

### Rendering with Transparency
The system uses OpenGL alpha blending:
- **Blend Mode**: `BLEND_ALPHA` for proper transparency
- **Backface Culling**: Disabled for double-sided visibility
- **Depth Testing**: Enabled for correct Z-ordering
- **Color**: White (255,255,255,255) to preserve original colors

## Usage Guide

### Creating Transparent PNGs

#### Option 1: Use Existing Transparent PNGs
Just drag and drop any PNG with transparency into the application.

#### Option 2: Create Your Own
Use image editing software:
- **Photoshop**: Save as PNG-24 or PNG-32
- **GIMP**: Export as PNG with alpha channel
- **Paint.NET**: Save as PNG (automatically includes alpha)
- **Online Tools**: Use remove.bg or similar for background removal

#### Requirements
- Format: PNG (not JPG, as JPG doesn't support transparency)
- Color Mode: RGB or RGBA
- Bit Depth: 24-bit (RGB) or 32-bit (RGBA)
- Alpha Channel: Present for transparency

### Adding Transparent Planes to Scene

**Method 1: Create New Plane**
```
1. Press 'A' to open Animation UI
2. Drag and drop transparent PNG file(s)
3. New plane created automatically
4. Transparent areas will be see-through
```

**Method 2: Add to Existing Plane**
```
1. Press 'A' to open Animation UI
2. Select a plane from the list
3. Drag and drop PNG file(s)
4. Frames added to selected plane
```

### Billboard Mode with Transparency
Billboard mode makes planes always face the camera - perfect for sprites!

```
1. Create or select an animated plane
2. In Animation UI, check "Billboard" checkbox
3. Plane rotates to always face camera
4. Transparency works perfectly in this mode
```

## Technical Details

### Blend Mode Implementation
```cpp
// Before drawing plane
rlDisableBackfaceCulling();  // See both sides
rlSetBlendMode(BLEND_ALPHA);  // Enable transparency

// Draw textured quad with RGBA color
rlColor4ub(255, 255, 255, 255);

// After drawing
rlEnableBackfaceCulling();  // Restore default
```

### Alpha Blending Formula
```
FinalColor = SourceColor * SourceAlpha + DestColor * (1 - SourceAlpha)
```
Where:
- `SourceColor`: PNG pixel color
- `SourceAlpha`: PNG pixel transparency (0-255)
- `DestColor`: Background/existing pixel color

### Rendering Order
1. Opaque geometry (walls, nodes)
2. Transparent planes (animated planes)
3. UI overlays

**Note**: Planes are drawn in order of creation, not depth-sorted. For complex scenes with overlapping transparent planes, create them back-to-front.

## Common Use Cases

### 1. Character Sprites
- Create character animations with transparent backgrounds
- Use billboard mode for camera-facing characters
- Animate with multiple PNG frames

### 2. UI Elements in 3D Space
- Health bars, name tags, markers
- Keep in world space but always visible
- Semi-transparent for HUD-like appearance

### 3. Particle Effects
- Fire, smoke, explosions
- Multiple frames for animation
- Transparent edges for smooth blending

### 4. Decals and Overlays
- Signs, posters, paintings on walls
- Position freely in 3D space
- Non-billboard mode for surface-aligned placement

### 5. Transparent Windows/Glass
- Single transparent PNG showing glass texture
- Position in architectural models
- Visible from both sides

## Examples

### Example 1: Animated Character
```
Files needed:
- character_idle_01.png (transparent background)
- character_idle_02.png (transparent background)
- character_idle_03.png (transparent background)

Steps:
1. Press 'A'
2. Drag all 3 files together
3. Check "Billboard" in Animation UI
4. Adjust frame time (e.g., 0.1 seconds)
5. Position character in scene
```

### Example 2: Floating Icon
```
File needed:
- icon.png (with alpha channel)

Steps:
1. Press 'A'
2. Drag icon.png
3. Check "Billboard"
4. Set size to 2x2 for smaller icon
5. Position above object
```

### Example 3: Semi-Transparent Overlay
```
File needed:
- overlay.png (semi-transparent, 50% opacity)

Steps:
1. Create PNG with 50% opacity in image editor
2. Drag into application
3. Leave "Billboard" unchecked
4. Resize to cover area
5. Position in front of geometry
```

## Troubleshooting

### Problem: PNG Not Transparent
**Causes:**
- PNG doesn't have alpha channel
- File is actually JPG renamed to PNG
- Alpha channel was flattened during export

**Solutions:**
✓ Verify PNG has transparency in image editor
✓ Re-export with alpha channel enabled
✓ Use PNG-24 or PNG-32 format
✓ Check "Save Transparency" or similar option

### Problem: Edges Look Jagged
**Causes:**
- Low resolution PNG
- No anti-aliasing in original image
- Texture filtering issues

**Solutions:**
✓ Use higher resolution PNG (2x or 4x)
✓ Enable anti-aliasing when creating PNG
✓ Ensure smooth alpha channel edges
✓ Use premultiplied alpha if available

### Problem: Black Halo Around Edges
**Causes:**
- PNG has black background under semi-transparent pixels
- Not using premultiplied alpha

**Solutions:**
✓ Re-export PNG with transparent or white background
✓ Use "matting" or "defringe" tools in image editor
✓ Ensure background layer is deleted, not just hidden

### Problem: Plane Appears Too Dark
**Causes:**
- PNG has dark colors
- Lighting affecting appearance
- Wrong blend mode

**Solutions:**
✓ Check PNG colors in image editor
✓ Brighten PNG if needed
✓ Ensure BLEND_ALPHA mode is used (automatic)

### Problem: Can Only See One Side
**Causes:**
- Backface culling enabled (shouldn't happen with fix)
- Camera inside the plane

**Solutions:**
✓ Code now disables backface culling automatically
✓ Move camera to see plane from desired angle
✓ Check both sides are being rendered

## Performance Notes

### Texture Size Recommendations
- **Small Icons**: 128x128 to 256x256 pixels
- **Character Sprites**: 512x512 to 1024x1024 pixels
- **Large Overlays**: 1024x1024 to 2048x2048 pixels
- **Maximum**: 4096x4096 (GPU dependent)

### Memory Usage
Each PNG frame is loaded into VRAM:
- 256x256 RGBA = ~256 KB
- 512x512 RGBA = ~1 MB
- 1024x1024 RGBA = ~4 MB
- 2048x2048 RGBA = ~16 MB

### Performance Impact
- Transparent rendering: Minimal overhead
- Multiple planes: Each plane is one draw call
- Animation: No performance cost (just texture swap)
- Billboard mode: Slight CPU cost for rotation calculation

## Best Practices

### Image Preparation
1. **Trim Excess**: Remove unnecessary transparent space
2. **Optimize Size**: Use smallest resolution that looks good
3. **Compress**: Use PNG optimization tools (e.g., TinyPNG)
4. **Test Alpha**: Verify transparency in image viewer first
5. **Premultiply**: Use premultiplied alpha if possible

### In-Application Usage
1. **Group Related**: Put animation frames in same plane
2. **Name Files**: Use descriptive names (char_walk_01.png)
3. **Organize**: Keep PNGs in project folder
4. **Test Save/Load**: Verify transparency persists after save
5. **Z-Order**: Create planes back-to-front for proper overlap

### Animation Tips
1. **Consistent Size**: All frames should be same dimensions
2. **Smooth Timing**: Adjust frame time for smooth animation
3. **Loop Seamlessly**: First and last frames should connect
4. **Use Sprites**: Single PNG with multiple frames works too
5. **Test Playback**: Preview animation before finalizing

## Saving and Loading

### Transparency Preservation
✅ Transparency is fully preserved when saving and loading projects:
1. PNG file paths are saved in .dat file
2. On load, textures are reloaded from original files
3. Alpha channel is preserved automatically
4. Planes appear exactly as before

### File Requirements
- Original PNG files must remain at saved paths
- Moving PNGs after save will break loading
- Keep PNGs in project folder for portability
- Both file path and texture data are preserved

## Keyboard Shortcuts

- **A**: Toggle Animation UI (manage planes)
- **CTRL+S**: Save project (preserves transparency)
- **CTRL+L**: Load project (restores transparency)
- **ESC**: Deselect plane

## Technical Specifications

### Supported Formats
- **PNG**: Full support with alpha channel ✅
- **JPG**: No transparency support (opaque) ⚠️
- **BMP**: No transparency support (opaque) ⚠️
- **TGA**: May support alpha (untested) ⚠️

### Rendering Pipeline
1. Load PNG with `LoadTexture()` (alpha preserved)
2. Store in `AnimatedPlane.frames` vector
3. Enable `BLEND_ALPHA` before drawing
4. Disable backface culling for double-sided
5. Draw front and back faces with texture
6. Restore default render state

### OpenGL Blend Settings
```
Source Factor: GL_SRC_ALPHA
Dest Factor: GL_ONE_MINUS_SRC_ALPHA
Equation: GL_FUNC_ADD
```

## FAQ

**Q: Do I need to do anything special to enable transparency?**
A: No! Just use PNG files with alpha channels. The system handles everything automatically.

**Q: Can I use semi-transparent PNGs (not just fully transparent)?**
A: Yes! All alpha values from 0 (fully transparent) to 255 (fully opaque) are supported.

**Q: Will transparency work in billboard mode?**
A: Yes! Billboard mode works perfectly with transparent PNGs.

**Q: Can I mix transparent and opaque textures?**
A: Yes! You can have some planes with transparent PNGs and others with opaque images.

**Q: Does transparency affect performance?**
A: Minimal impact. Transparent rendering is well-optimized in modern GPUs.

**Q: Can I see the plane from both sides?**
A: Yes! The system renders both front and back faces, so planes are visible from any angle.

**Q: What if my PNG has a white background?**
A: The white will show up. You need a PNG with a transparent background (alpha channel).

**Q: How do I remove a background from an existing image?**
A: Use tools like Photoshop, GIMP, or online services like remove.bg to remove backgrounds.

## Summary

✅ **Transparent PNG support is fully implemented**
✅ **No special setup required - just drag and drop**
✅ **Alpha blending enabled automatically**
✅ **Double-sided rendering for visibility**
✅ **Works in both normal and billboard modes**
✅ **Saves and loads correctly**
✅ **Performance optimized**

Enjoy creating beautiful transparent overlays and animations!