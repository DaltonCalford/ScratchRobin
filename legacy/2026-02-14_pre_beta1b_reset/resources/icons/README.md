# ScratchRobin Icon System

## Overview

This directory contains all icons used in the ScratchRobin application. Icons are stored as SVG source files and rendered to PNG at multiple sizes (16px, 24px, 32px, 48px, 64px, 128px, 256px).

## Icon Inventory

### Application & Navigation (0-9)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 0 | scratchrobin | scratchrobin.svg | Application root |
| 1 | connect | connect.svg | Database connection |
| 2 | settings | settings.svg | Settings/configuration |
| 3 | stop | stop.svg | Error/alert |
| 4 | diagram | diagram.svg | ERD/diagram |
| 5 | bookmark | bookmark.svg | Bookmark/favorite |
| 6 | tag | tag.svg | Tag/label |

### Database Objects (10-24)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 10 | table | table.svg | Database table |
| 11 | view | view.svg | Database view |
| 12 | column | column.svg | Table column |
| 13 | index | index.svg | Table index |
| 14 | sequence | sequence.svg | Sequence/generator |
| 15 | trigger | trigger.svg | Database trigger |
| 16 | constraint | constraint.svg | Table constraint |
| 17 | procedure | procedure.svg | Stored procedure |
| 18 | function | function.svg | Function/UDF |
| 19 | package | package.svg | Package/module |
| 20 | domain | domain.svg | Domain/type |
| 21 | collation | collation.svg | Collation/charset |
| 22 | tablespace | tablespace.svg | Tablespace/storage |

### Schema Organization (25-29)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 25 | database | diagram.svg | Database |
| 26 | catalog | diagram.svg | Catalog |
| 27 | schema | folder.svg | Schema |
| 28 | folder | folder.svg | Folder/group |

### Security & Admin (30-34)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 30 | users | users.svg | User/role |
| 31 | host | server.svg | Host/server |
| 32 | permission | settings.svg | Permission |
| 33 | audit | settings.svg | Audit log |
| 34 | history | history.svg | History/archive |

### Project Objects (35-44)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 35 | project | project.svg | Project/workspace |
| 36 | sql | sql.svg | SQL script |
| 37 | note | note.svg | Note/documentation |
| 38 | timeline | timeline.svg | Timeline/workflow |
| 39 | job | job.svg | Scheduled job |

### Version Control (45-49)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 45 | git | git.svg | Git repository |
| 46 | branch | branch.svg | Git branch |

### Maintenance (50-54)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 50 | backup | backup.svg | Backup |
| 51 | restore | restore.svg | Restore |

### Infrastructure (60-69)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 60 | server | server.svg | Database server |
| 61 | client | client.svg | Client/workstation |
| 62 | filespace | filespace.svg | Storage/filespace |
| 63 | network | network.svg | Network |
| 64 | cluster | cluster.svg | Cluster |
| 65 | instance | instance.svg | DB instance |
| 66 | replica | replica.svg | Replica/slave |
| 67 | shard | shard.svg | Shard/partition |

### Design & Planning (70-79)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 70 | whiteboard | whiteboard.svg | Whiteboard |
| 71 | mindmap | mindmap.svg | Mind map |
| 72 | design | design.svg | Design document |
| 73 | draft | draft.svg | Draft/work in progress |
| 74 | template | template.svg | Template |
| 75 | blueprint | blueprint.svg | Blueprint/plan |
| 76 | concept | concept.svg | Concept/idea |
| 77 | plan | plan.svg | Implementation plan |

### Design States (80-89)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 80 | implemented | implemented.svg | Implemented/deployed |
| 81 | pending | pending.svg | Pending/awaiting review |
| 82 | modified | modified.svg | Modified/changed |
| 83 | deleted | deleted.svg | Deleted/removed |
| 84 | newitem | newitem.svg | New item |

### Synchronization (90-99)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 90 | sync | sync.svg | Synchronize |
| 91 | diff | diff.svg | Diff/compare |
| 92 | compare | compare.svg | Comparison |
| 93 | migrate | migrate.svg | Migration |
| 94 | deploy | deploy.svg | Deployment |

### Collaboration (100-109)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 100 | shared | shared.svg | Shared resource |
| 101 | collaboration | collaboration.svg | Collaboration |
| 102 | team | team.svg | Team |

### Security States (110-119)
| Index | Name | File | Description |
|-------|------|------|-------------|
| 110 | lock | lock.svg | Locked/protected |
| 111 | unlock | unlock.svg | Unlocked |
| 112 | history | history.svg | History |
| 113 | audit | audit.svg | Audit |
| 114 | tag | tag.svg | Tag |
| 115 | bookmark | bookmark.svg | Bookmark |

### Default
| Index | Name | File | Description |
|-------|------|------|-------------|
| 120 | default | table.svg | Default/unknown |

## Total: 68 Unique Icons

## Adding New Icons

1. **Create SVG**: Create a new SVG file in this directory
   - Recommended size: 16x16 viewBox
   - Keep it simple and recognizable at small sizes
   - Use consistent styling with existing icons

2. **Add to generate script**: Edit `generate-pngs.sh` and add the icon name to the `ICONS` array

3. **Generate PNGs**: Run the generation script:
   ```bash
   ./generate-pngs.sh
   ```

4. **Update code**: In `main_frame.cpp`:
   - Add `loadIcon()` call in `InitializeTreeIcons()`
   - Add mapping in `GetIconIndexForNode()`

5. **Update documentation**: Update this README with the new icon

## Icon Design Guidelines

### Colors
- **Database objects**: Blues, grays
- **Design elements**: Purples, oranges
- **States**: Green (good), yellow (warning), red (error)
- **Infrastructure**: Teals, browns
- **Actions**: Matching action color (green for go, red for stop)

### Style
- Simple, flat design
- 1-2px strokes for lines
- Solid fills for shapes
- Readable at 16x16 pixels
- Consistent visual weight

### Tools
- Inkscape (free, recommended)
- Adobe Illustrator
- Figma
- Any SVG editor

## SVG Template

```svg
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">
  <!-- Your icon content here -->
  <rect x="2" y="2" width="12" height="12" fill="#color" stroke="#stroke" stroke-width="1"/>
</svg>
```

## Generating PNGs

Requires ImageMagick or librsvg:

```bash
# Ubuntu/Debian
sudo apt-get install imagemagick

# Or install librsvg for better SVG rendering
sudo apt-get install librsvg2-bin

# Run generation
./generate-pngs.sh
```

## Usage in Code

```cpp
// In InitializeTreeIcons():
loadIcon("myicon", wxART_DEFAULT);

// In GetIconIndexForNode():
if (kind == "myobject") return INDEX;  // Use appropriate index
```

## License

Icons are part of ScratchRobin and licensed under the same terms.
