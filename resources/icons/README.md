# ScratchRobin Icon Assets

This directory contains the icon assets for ScratchRobin.

## File Organization

### Source Files (SVG)
Canonical source files in SVG format:

| Icon | Description |
|------|-------------|
| `scratchrobin.svg` | Application icon (256x256) |
| `connect.svg` | Connect to database |
| `disconnect.svg` | Disconnect from database |
| `execute.svg` | Execute SQL query |
| `stop.svg` | Cancel/stop operation |
| `commit.svg` | Commit transaction |
| `rollback.svg` | Rollback transaction |
| `table.svg` | Table object |
| `view.svg` | View object |
| `diagram.svg` | ERD diagram |
| `index.svg` | Database index |
| `users.svg` | Users/roles |
| `settings.svg` | Preferences/settings |

### Generated Files (PNG)
PNG files are generated at the following sizes:
- `@16.png` - Menu items, small buttons
- `@24.png` - Toolbar (default)
- `@32.png` - Toolbar (high DPI)
- `@48.png` - Large buttons, lists
- `@64.png` - Dialogs
- `@128.png` - Application icon (Windows)
- `@256.png` - Application icon (macOS, Linux)

## Generating PNGs

To generate PNG files from SVG sources:

```bash
cd resources/icons
./generate-pngs.sh
```

Requirements:
- `rsvg-convert` (from librsvg) - recommended, better SVG support
- OR ImageMagick's `convert`

## Design Guidelines

- Use flat design style
- Stick to the project's color palette:
  - Primary: #3498db (Blue)
  - Success: #27ae60 (Green)
  - Warning: #f39c12 (Orange)
  - Error: #e74c3c (Red)
  - Neutral: #7f8c8d (Gray)
- Icons should be readable at 24x24 pixels
- Use consistent stroke widths and corner radii

## Adding New Icons

1. Create SVG file following naming convention: `action-name.svg`
2. Add to `generate-pngs.sh` ICONS array
3. Run generation script
4. Update this README
