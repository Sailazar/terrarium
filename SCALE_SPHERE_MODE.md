# Scale Sphere Mode - Documentation

## Overview

**MODE_SCALE_SPHERE** (Mode 8) is a specialized mode that allows you to change the visual size of selected node spheres without affecting their positions or the geometry of your 3D structure.

---

## What's the Difference?

### MODE_SCALE (Mode 7) - Position Scaling
- Scales the **positions** of nodes relative to a center point
- Changes the **geometry** and **shape** of your structure
- Nodes move closer or further apart
- Use this when you want to make your entire structure bigger or smaller

### MODE_SCALE_SPHERE (Mode 8) - Visual Scaling
- Scales only the **visual size** of the sphere representations
- Does **NOT** change positions or geometry
- Nodes stay in the exact same place
- Use this for visual emphasis, hierarchy, or design purposes

---

## How to Use

### Step 1: Select Nodes
1. Press `[1]` to enter SELECT MODE
2. Click on nodes to select them (they turn yellow)
3. You can select multiple nodes

### Step 2: Enter Scale Sphere Mode
- Press `[8]` to enter SCALE SPHERE MODE
- The mode indicator will show "SCALE SPHERE MODE"
- Selected spheres are ready to be scaled

### Step 3: Scale the Spheres

**Keyboard Controls:**
- `[+]` or `[=]` - Increase sphere size
- `[-]` or `[_]` - Decrease sphere size

**Mouse Wheel:**
- Scroll **UP** - Increase sphere size
- Scroll **DOWN** - Decrease sphere size

**Mouse Drag:**
- Hold `[LMB]` and drag mouse **UP** - Increase sphere size
- Hold `[LMB]` and drag mouse **DOWN** - Decrease sphere size

### Step 4: Done!
- Sphere sizes are updated immediately
- Changes are saved to undo history
- Positions remain unchanged

---

## Scale Range

**Minimum:** 0.1 (10% of default size)  
**Maximum:** 5.0 (500% of default size)  
**Default:** 1.0 (100% - normal size)  
**Step Size:** 0.1 per keypress  
**Mouse Sensitivity:** 0.01 per pixel

---

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `8` | Enter SCALE SPHERE MODE |
| `1` | Return to SELECT MODE (to select different nodes) |
| `+` / `=` | Increase sphere size |
| `-` / `_` | Decrease sphere size |
| `Mouse Wheel` | Smooth scaling |
| `LMB + Drag` | Interactive scaling |
| `CTRL+Z` | Undo last scale change |

---

## Use Cases

### Visual Hierarchy
Make important nodes larger to emphasize key points in your structure:
```
Hub node: scale = 2.0 (large)
Connection nodes: scale = 1.0 (normal)
Minor nodes: scale = 0.5 (small)
```

### Detail Level
Scale down nodes in areas with high density to reduce visual clutter:
```
Dense area: scale = 0.3 (very small)
Normal area: scale = 1.0 (normal)
```

### Node Types
Use different sizes to represent different node types:
```
Type A (structural): scale = 1.5
Type B (decorative): scale = 0.8
Type C (temporary): scale = 0.4
```

### Selection Emphasis
Temporarily scale up selected nodes for easier visibility:
```
1. Select nodes
2. Press [8]
3. Scale up to 2.0
4. Work on connections
5. Scale back to 1.0
```

---

## Examples

### Example 1: Emphasize Corner Nodes
```
Goal: Make corner nodes of a cube larger

1. Press [1] (SELECT MODE)
2. Click all 8 corner nodes
3. Press [8] (SCALE SPHERE MODE)
4. Press [+] three times (scale to ~1.3)
5. Result: Corner nodes are now more visible
```

### Example 2: Create Visual Depth
```
Goal: Nodes further from camera appear smaller

1. Select distant nodes (press [1])
2. Press [8] (SCALE SPHERE MODE)
3. Press [-] to make them smaller
4. Select nearby nodes
5. Press [+] to make them larger
6. Result: Enhanced depth perception
```

### Example 3: Node Hierarchy System
```
Goal: 3-tier size system

Tier 1 (Main nodes):
  - Select main structural nodes
  - Press [8], scale to 2.0

Tier 2 (Secondary nodes):
  - Select secondary nodes
  - Press [8], scale to 1.0 (leave default)

Tier 3 (Detail nodes):
  - Select detail nodes
  - Press [8], scale to 0.5
```

---

## Tips & Best Practices

### Visual Design
✓ Use scale to create visual hierarchy  
✓ Keep scale differences meaningful (0.5x, 1.0x, 2.0x)  
✓ Maintain consistency within node groups  
✓ Use subtle scaling (0.8-1.2) for minor variations  
✓ Use dramatic scaling (0.3-3.0) for emphasis  

### Performance
✓ Scaling spheres has no performance impact  
✓ All scale values are stored per-node  
✓ No geometry recalculation needed  
✓ Instant rendering update  

### Workflow
✓ Scale spheres after positioning is finalized  
✓ Use SELECT mode to switch between node groups  
✓ Combine with other modes for complete editing  
✓ Use CTRL+Z if you make a mistake  

### Avoid
✗ Don't scale to 0.1 unless intentional (nearly invisible)  
✗ Don't mix random scales without purpose  
✗ Don't use scale for actual geometry changes (use Mode 7)  
✗ Don't forget some nodes may be too small to select later  

---

## Technical Details

### Storage
- Scale value stored as `float` in `Node` struct
- Default value: `1.0f`
- Range: `0.1f` to `5.0f`
- Precision: Float (single precision)

### Rendering
```cpp
DrawSphere(node.position, sphereRadius * node.scale, color);
```

Where:
- `sphereRadius` = base radius (typically 0.15)
- `node.scale` = per-node scale multiplier
- Final radius = `sphereRadius * node.scale`

### Undo Support
- Each scaling action is saved to undo history
- Mouse drag saves once on release
- Keyboard/wheel saves immediately
- Can undo multiple scale operations

### Code Location
- Mode definition: `test.cpp` line ~656
- Implementation: `test.cpp` lines ~1785-1871
- Mode switching: `test.cpp` line ~813
- UI display: `test.cpp` lines ~2033-2041

---

## Comparison with Mode 7

| Feature | Mode 7 (Scale) | Mode 8 (Scale Sphere) |
|---------|---------------|---------------------|
| Changes positions | ✓ Yes | ✗ No |
| Changes geometry | ✓ Yes | ✗ No |
| Changes visual size | ✗ No | ✓ Yes |
| Affects connections | ✓ Yes | ✗ No |
| Affects walls | ✓ Yes | ✗ No |
| Scale from center | ✓ Yes | N/A |
| Range | Unlimited | 0.1 - 5.0 |
| Use case | Resize structure | Visual emphasis |

---

## Troubleshooting

**Problem:** "No spheres selected!"  
**Solution:** Press [1] to enter SELECT mode and click nodes first

**Problem:** Spheres are too small to see  
**Solution:** Select all (drag select) and scale up with [+] or mouse wheel

**Problem:** Spheres are too large  
**Solution:** Select them and scale down with [-]

**Problem:** Can't select tiny spheres  
**Solution:** Use drag-select rectangle or increase scale

**Problem:** Scale not changing  
**Solution:** Make sure you're in SCALE SPHERE mode (press [8])

**Problem:** Accidental scaling  
**Solution:** Press CTRL+Z to undo

---

## Color Scheme

In the UI, SCALE SPHERE mode uses:
- **Mode Color:** `UI_TEXT_PRIMARY` (Light Cyan #80D4FF)
- **Text Color:** Same as mode color
- **Purpose:** Differentiates from geometric scale (Amber)

---

## FAQ

**Q: Does scaling spheres affect physics or collisions?**  
A: No, this is purely visual. Node positions remain unchanged.

**Q: Can I scale individual spheres differently?**  
A: Yes! Select one node at a time and scale each differently.

**Q: Will scaled spheres export to OBJ?**  
A: No, OBJ export uses node positions only. Scale is for visual editing.

**Q: Can I animate sphere sizes?**  
A: Not currently, but you could script it by modifying node.scale values.

**Q: Does sphere size affect connections?**  
A: No, connections are drawn between node positions regardless of sphere size.

**Q: What happens if I scale to maximum (5.0)?**  
A: Spheres become very large but remain functional. May obscure other nodes.

**Q: Can I reset all scales to default (1.0)?**  
A: Select all nodes, press [8], then manually adjust. No auto-reset currently.

**Q: Does this mode work with all node types?**  
A: Yes, all nodes have a scale property regardless of module or type.

---

## Future Enhancements

Potential features for future versions:
- [ ] Batch scale reset to 1.0
- [ ] Scale presets (0.5, 1.0, 2.0 shortcuts)
- [ ] Scale by node depth/distance
- [ ] Scale interpolation between selected nodes
- [ ] Scale animation over time
- [ ] Scale based on connection count
- [ ] Visual scale indicators (rings, halos)
- [ ] Scale groups (named scale sets)

---

## See Also

- **MODE_SCALE (Mode 7)** - For geometric scaling
- **SELECT MODE (Mode 1)** - For selecting nodes
- **README.md** - Complete controls reference
- **TEXTURE_GUIDE.md** - Texture system documentation
- **COLOR_SCHEME.md** - UI color palette

---

**Version:** 1.0  
**Added:** January 7, 2025  
**Mode Number:** 8  
**Keyboard Shortcut:** `[8]`  
**Status:** ✅ Fully Implemented

---

**Quick Reference:**
```
Press [8] → Scale Sphere Mode
Select nodes → Press [1], click nodes
Scale up → [+] or Mouse Wheel Up or LMB Drag Up
Scale down → [-] or Mouse Wheel Down or LMB Drag Down
Range → 0.1 to 5.0 (default 1.0)
```
