# ScratchRobin Icon Specification

Complete list of icons needed for the application, organized by category with all required states.

## Icon States Reference

Each icon should have these states where applicable:

| State | Description | Visual Treatment |
|-------|-------------|------------------|
| **normal** | Default appearance | Full color, standard opacity |
| **disabled** | Cannot interact | Grayscale, 40% opacity, no shadow |
| **hover** | Mouse over | Slight brightness increase, subtle glow |
| **pressed** | Mouse down/clicked | Darker shade, inset shadow |
| **selected** | Active/toggled on | Accent color background, white icon |
| **focused** | Keyboard navigation | Outline/border highlight |

## 1. Application Icons (Logo & Branding)

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `app_logo` | Main application logo (robin bird with database cylinder) | normal, splash |
| `app_icon_16` | 16x16 window icon | normal |
| `app_icon_32` | 32x32 window icon | normal |
| `app_icon_48` | 48x48 taskbar icon | normal |
| `app_icon_256` | 256x256 high-res icon | normal |

**Design Notes:**
- Style: Modern flat with subtle depth
- Primary colors: Robin orange (#FF6B35), Database blue (#2E86AB)
- Background: Transparent or circular

---

## 2. File Operations

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `file_new` | Blank document with plus | normal, disabled, hover, pressed |
| `file_open` | Folder opening | normal, disabled, hover, pressed |
| `file_save` | Floppy disk | normal, disabled, hover, pressed |
| `file_save_as` | Floppy disk with pencil | normal, disabled, hover, pressed |
| `file_save_all` | Multiple floppy disks | normal, disabled, hover, pressed |
| `file_close` | Document with X | normal, disabled, hover, pressed |
| `file_close_all` | Multiple documents with X | normal, disabled, hover, pressed |
| `file_print` | Printer | normal, disabled, hover, pressed |
| `file_print_preview` | Printer with magnifier | normal, disabled, hover, pressed |
| `file_export` | Document with arrow out | normal, disabled, hover, pressed |
| `file_import` | Document with arrow in | normal, disabled, hover, pressed |
| `file_properties` | Document with gear | normal, disabled, hover, pressed |

---

## 3. Edit Operations

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `edit_undo` | Curved left arrow | normal, disabled, hover, pressed |
| `edit_redo` | Curved right arrow | normal, disabled, hover, pressed |
| `edit_cut` | Scissors | normal, disabled, hover, pressed |
| `edit_copy` | Two overlapping documents | normal, disabled, hover, pressed |
| `edit_paste` | Clipboard with document | normal, disabled, hover, pressed |
| `edit_delete` | Trash can | normal, disabled, hover, pressed |
| `edit_delete_forever` | Trash can with X | normal, disabled, hover, pressed |
| `edit_select_all` | Document with checkmarks | normal, disabled, hover, pressed |
| `edit_find` | Magnifying glass | normal, disabled, hover, pressed |
| `edit_find_replace` | Magnifying glass with arrows | normal, disabled, hover, pressed |
| `edit_preferences` | Gear/cog | normal, disabled, hover, pressed |
| `edit_clear` | Eraser | normal, disabled, hover, pressed |

---

## 4. Database Connection Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `connection_new` | Plug with plus | normal, disabled, hover, pressed |
| `connection_edit` | Plug with pencil | normal, disabled, hover, pressed |
| `connection_delete` | Plug with X | normal, disabled, hover, pressed |
| `connection_connect` | Plug connecting | normal, disabled, hover, pressed |
| `connection_disconnect` | Unplugged | normal, disabled, hover, pressed |
| `connection_test` | Plug with checkmark | normal, disabled, hover, pressed |
| `connection_refresh` | Circular arrows around plug | normal, disabled, hover, pressed, loading |
| `connection_secure` | Plug with lock | normal, disabled, hover, pressed |
| `connection_error` | Plug with warning triangle | normal, hover |
| `connection_loading` | Animated connecting dots | normal (animated) |

**Status Indicators:**
| Icon Name | Description | States |
|-----------|-------------|--------|
| `status_connected` | Green dot | normal |
| `status_disconnected` | Gray dot | normal |
| `status_connecting` | Yellow/orange pulsing dot | normal (animated) |
| `status_error` | Red dot | normal |

---

## 5. Database Object Icons

### 5.1 Core Objects

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `db_database` | Cylinder (database symbol) | normal, selected, disabled |
| `db_schema` | Cylinder with layer | normal, selected, disabled |
| `db_table` | Grid/spreadsheet | normal, selected, disabled, system |
| `db_view` | Grid with eye | normal, selected, disabled, system |
| `db_column` | Vertical line with letter C | normal, selected, disabled |
| `db_index` | Grid with lightning bolt | normal, selected, disabled |
| `db_constraint_pk` | Grid with key | normal, selected, disabled |
| `db_constraint_fk` | Grid with chain link | normal, selected, disabled |
| `db_constraint_unique` | Grid with U | normal, selected, disabled |
| `db_constraint_check` | Grid with checkmark | normal, selected, disabled |
| `db_procedure` | Document with gear and DB | normal, selected, disabled |
| `db_function` | Document with fx | normal, selected, disabled |
| `db_trigger` | Lightning bolt hitting table | normal, selected, disabled |
| `db_sequence` | Numbers with arrows | normal, selected, disabled |

### 5.2 Special Objects

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `db_synonym` | Table with alias arrow | normal, selected, disabled |
| `db_package` | Folder with DB symbol | normal, selected, disabled |
| `db_type` | Cube with T | normal, selected, disabled |
| `db_domain` | Circle with D inside | normal, selected, disabled |
| `db_lob` | Document with blob shape | normal, selected, disabled |
| `db_event` | Calendar with bell | normal, selected, disabled |
| `db_job` | Clock with gear | normal, selected, disabled |

### 5.3 Object Variants (Type Indicators)

| Icon Name | Description | States |
|-----------|-------------|--------|
| `db_table_system` | Table with gear overlay | normal |
| `db_table_temp` | Table with clock overlay | normal |
| `db_table_external` | Table with cloud overlay | normal |
| `db_table_partitioned` | Table with sections | normal |
| `db_view_materialized` | View with refresh arrow | normal |
| `db_procedure_external` | Procedure with cloud | normal |

---

## 6. SQL Editor Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `sql_execute` | Play button (triangle) | normal, disabled, hover, pressed |
| `sql_execute_selection` | Play with highlighted lines | normal, disabled, hover, pressed |
| `sql_explain` | Play with question mark | normal, disabled, hover, pressed |
| `sql_stop` | Square stop | normal, disabled, hover, pressed |
| `sql_format` | Text with magic wand | normal, disabled, hover, pressed |
| `sql_comment` | Text with // | normal, disabled, hover, pressed |
| `sql_uncomment` | Text without // | normal, disabled, hover, pressed |
| `sql Beautify` | Text with sparkle | normal, disabled, hover, pressed |
| `sql_history` | Clock with SQL | normal, disabled, hover, pressed |
| `sql_bookmark` | Star on document | normal, disabled, hover, pressed, selected |
| `sql_bookmark_add` | Star outline with plus | normal, disabled, hover, pressed |
| `sql_bookmark_remove` | Star with X | normal, disabled, hover, pressed |
| `sql_parameter` | SQL with input box | normal, disabled, hover, pressed |
| `sql_snippet` | SQL puzzle piece | normal, disabled, hover, pressed |
| `sql_compare` | Two SQL documents | normal, disabled, hover, pressed |
| `sql_import` | SQL with down arrow | normal, disabled, hover, pressed |
| `sql_export` | SQL with up arrow | normal, disabled, hover, pressed |

---

## 7. Data Grid/Table Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `grid_first` | Double left arrow to line | normal, disabled, hover, pressed |
| `grid_previous` | Left arrow | normal, disabled, hover, pressed |
| `grid_next` | Right arrow | normal, disabled, hover, pressed |
| `grid_last` | Double right arrow to line | normal, disabled, hover, pressed |
| `grid_refresh` | Circular arrow | normal, disabled, hover, pressed, loading |
| `grid_filter` | Funnel | normal, disabled, hover, pressed, active |
| `grid_filter_clear` | Funnel with X | normal, disabled, hover, pressed |
| `grid_sort_asc` | Arrow up with A-Z | normal, disabled, hover, pressed, selected |
| `grid_sort_desc` | Arrow down with Z-A | normal, disabled, hover, pressed, selected |
| `grid_sort_clear` | Arrow with X | normal, disabled, hover, pressed |
| `grid_edit` | Grid with pencil | normal, disabled, hover, pressed |
| `grid_add_row` | Grid with plus | normal, disabled, hover, pressed |
| `grid_delete_row` | Grid with minus | normal, disabled, hover, pressed |
| `grid_duplicate_row` | Grid with copy | normal, disabled, hover, pressed |
| `grid_commit` | Checkmark | normal, disabled, hover, pressed |
| `grid_rollback` | Curved back arrow | normal, disabled, hover, pressed |
| `grid_search` | Magnifier on grid | normal, disabled, hover, pressed |
| `grid_export_csv` | Grid with CSV | normal, disabled, hover, pressed |
| `grid_export_excel` | Grid with XLS | normal, disabled, hover, pressed |
| `grid_export_json` | Grid with JSON | normal, disabled, hover, pressed |
| `grid_print` | Grid with printer | normal, disabled, hover, pressed |
| `grid_transpose` | Grid rotated 90° | normal, disabled, hover, pressed |
| `grid_freeze_column` | Grid with frozen section | normal, disabled, hover, pressed, selected |
| `grid_group` | Grid with group bracket | normal, disabled, hover, pressed, selected |
| `grid_ungroup` | Grid without bracket | normal, disabled, hover, pressed |

---

## 8. Diagram/ERD Icons

### 8.1 Tools

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `diagram_select` | Mouse pointer | normal, disabled, hover, pressed, selected |
| `diagram_pan` | Hand grabbing | normal, disabled, hover, pressed, selected |
| `diagram_zoom_in` | Plus with magnifier | normal, disabled, hover, pressed |
| `diagram_zoom_out` | Minus with magnifier | normal, disabled, hover, pressed |
| `diagram_zoom_fit` | Square fitting frame | normal, disabled, hover, pressed |
| `diagram_zoom_100` | 1:1 ratio | normal, disabled, hover, pressed |
| `diagram_zoom_all` | Frame with arrows out | normal, disabled, hover, pressed |

### 8.2 Entity Tools

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `diagram_add_table` | Table with plus | normal, disabled, hover, pressed |
| `diagram_add_view` | View with plus | normal, disabled, hover, pressed |
| `diagram_add_note` | Sticky note with plus | normal, disabled, hover, pressed |
| `diagram_add_image` | Picture with plus | normal, disabled, hover, pressed |

### 8.3 Relationship Tools

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `diagram_relation_11` | Two tables with 1:1 | normal, disabled, hover, pressed, selected |
| `diagram_relation_1n` | Two tables with 1:N | normal, disabled, hover, pressed, selected |
| `diagram_relation_nm` | Two tables with N:M | normal, disabled, hover, pressed, selected |
| `diagram_relation_inheritance` | Tables with triangle | normal, disabled, hover, pressed, selected |
| `diagram_relation_identifying` | Solid line key | normal, disabled, hover, pressed |
| `diagram_relation_non_identifying` | Dashed line key | normal, disabled, hover, pressed |

### 8.4 Layout Tools

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `diagram_layout_auto` | Auto-arrange symbol | normal, disabled, hover, pressed |
| `diagram_layout_hierarchical` | Top-down tree | normal, disabled, hover, pressed, selected |
| `diagram_layout_organic` | Scattered connected dots | normal, disabled, hover, pressed, selected |
| `diagram_layout_circular` | Circle arrangement | normal, disabled, hover, pressed, selected |
| `diagram_align_left` | Lines aligned left | normal, disabled, hover, pressed |
| `diagram_align_center` | Lines centered | normal, disabled, hover, pressed |
| `diagram_align_right` | Lines aligned right | normal, disabled, hover, pressed |
| `diagram_align_top` | Horizontal lines top | normal, disabled, hover, pressed |
| `diagram_align_middle` | Horizontal lines middle | normal, disabled, hover, pressed |
| `diagram_align_bottom` | Horizontal lines bottom | normal, disabled, hover, pressed |
| `diagram_distribute_h` | Horizontal spacing | normal, disabled, hover, pressed |
| `diagram_distribute_v` | Vertical spacing | normal, disabled, hover, pressed |

### 8.5 View Options

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `diagram_show_grid` | Grid toggle | normal, hover, pressed, selected |
| `diagram_show_page_breaks` | Page boundaries | normal, hover, pressed, selected |
| `diagram_snap_to_grid` | Magnet on grid | normal, hover, pressed, selected |
| `diagram_notation_crowsfoot` | Crow's foot symbol | normal, hover, pressed, selected |
| `diagram_notation_chen` | Chen notation symbol | normal, hover, pressed, selected |
| `diagram_notation_uml` | UML diamond | normal, hover, pressed, selected |

### 8.6 Actions

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `diagram_reverse_engineer` | Database to diagram | normal, disabled, hover, pressed |
| `diagram_forward_engineer` | Diagram to database | normal, disabled, hover, pressed |
| `diagram_sync` | Bidirectional arrows DB<->Diagram | normal, disabled, hover, pressed |
| `diagram_validate` | Checkmark on diagram | normal, disabled, hover, pressed |
| `diagram_generate_sql` | Diagram with SQL | normal, disabled, hover, pressed |
| `diagram_compare` | Two diagrams | normal, disabled, hover, pressed |
| `diagram_merge` | Diagrams merging | normal, disabled, hover, pressed |
| `diagram_lock` | Padlock on diagram | normal, disabled, hover, pressed, selected |
| `diagram_unlock` | Open padlock | normal, disabled, hover, pressed |

---

## 9. Schema Compare/Apply Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `compare_source` | Left half | normal, disabled, hover, pressed |
| `compare_target` | Right half | normal, disabled, hover, pressed |
| `compare_bidirectional` | Two-way arrows | normal, disabled, hover, pressed |
| `diff_added` | Plus in green | normal |
| `diff_removed` | Minus in red | normal |
| `diff_modified` | Delta symbol yellow | normal |
| `diff_equal` | Equals sign gray | normal |
| `diff_conflict` | Exclamation mark red | normal |
| `apply_left` | Arrow pointing left | normal, disabled, hover, pressed |
| `apply_right` | Arrow pointing right | normal, disabled, hover, pressed |
| `apply_all` | Double arrows | normal, disabled, hover, pressed |
| `generate_script` | Document with code | normal, disabled, hover, pressed |
| `preview_changes` | Eye with delta | normal, disabled, hover, pressed |

---

## 10. Backup/Restore Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `backup_create` | Database with down arrow | normal, disabled, hover, pressed |
| `backup_restore` | Database with up arrow | normal, disabled, hover, pressed |
| `backup_schedule` | Clock with backup | normal, disabled, hover, pressed |
| `backup_full` | Complete cylinder | normal, disabled, hover, pressed, selected |
| `backup_incremental` | Cylinder with plus | normal, disabled, hover, pressed, selected |
| `backup_differential` | Cylinder with delta | normal, disabled, hover, pressed, selected |
| `backup_compress` | Cylinder squeezed | normal, disabled, hover, pressed |
| `backup_encrypt` | Cylinder with lock | normal, disabled, hover, pressed |
| `backup_verify` | Checkmark on backup | normal, disabled, hover, pressed |
| `backup_cloud` | Cloud with database | normal, disabled, hover, pressed |
| `backup_local` | Computer with database | normal, disabled, hover, pressed |

---

## 11. User/Security Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `user_add` | Person with plus | normal, disabled, hover, pressed |
| `user_edit` | Person with pencil | normal, disabled, hover, pressed |
| `user_delete` | Person with X | normal, disabled, hover, pressed |
| `user_group` | Multiple people | normal, disabled, hover, pressed |
| `user_role` | Person with badge | normal, disabled, hover, pressed |
| `user_permission` | Person with checkmark | normal, disabled, hover, pressed |
| `user_locked` | Person with lock | normal, disabled, hover, pressed |
| `user_admin` | Person with star | normal, disabled, hover, pressed |
| `role_add` | Badge with plus | normal, disabled, hover, pressed |
| `role_edit` | Badge with pencil | normal, disabled, hover, pressed |
| `privilege_grant` | Checkmark key | normal, disabled, hover, pressed |
| `privilege_revoke` | X over key | normal, disabled, hover, pressed |
| `security_audit` | Shield with list | normal, disabled, hover, pressed |
| `security_encrypt` | Lock with key | normal, disabled, hover, pressed |
| `security_mask` | Eye with slash | normal, disabled, hover, pressed |

---

## 12. Monitoring Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `monitor_dashboard` | Gauge/meters | normal, disabled, hover, pressed |
| `monitor_performance` | Speedometer | normal, disabled, hover, pressed |
| `monitor_queries` | SQL with clock | normal, disabled, hover, pressed |
| `monitor_connections` | Connected users | normal, disabled, hover, pressed |
| `monitor_locks` | Multiple padlocks | normal, disabled, hover, pressed |
| `monitor_deadlock` | Broken chain | normal, disabled, hover, pressed |
| `monitor_alert` | Bell with exclamation | normal, disabled, hover, pressed |
| `monitor_chart_line` | Line graph | normal, disabled, hover, pressed |
| `monitor_chart_bar` | Bar chart | normal, disabled, hover, pressed |
| `monitor_chart_pie` | Pie chart | normal, disabled, hover, pressed |
| `monitor_log` | Document with lines | normal, disabled, hover, pressed |
| `monitor_error_log` | Document with X | normal, disabled, hover, pressed |
| `monitor_slow_query` | Snail with SQL | normal, disabled, hover, pressed |
| `monitor_kill_process` | Target with X | normal, disabled, hover, pressed |

---

## 13. Import/Export Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `import_file` | Arrow into document | normal, disabled, hover, pressed |
| `import_table` | Arrow into table | normal, disabled, hover, pressed |
| `import_wizard` | Magic wand with arrow | normal, disabled, hover, pressed |
| `export_file` | Arrow out of document | normal, disabled, hover, pressed |
| `export_table` | Arrow out of table | normal, disabled, hover, pressed |
| `export_csv` | Document with CSV | normal, disabled, hover, pressed |
| `export_excel` | Document with XLS | normal, disabled, hover, pressed |
| `export_json` | Document with JSON | normal, disabled, hover, pressed |
| `export_xml` | Document with XML | normal, disabled, hover, pressed |
| `export_pdf` | Document with PDF | normal, disabled, hover, pressed |
| `export_sql` | Document with SQL | normal, disabled, hover, pressed |
| `export_html` | Document with HTML | normal, disabled, hover, pressed |
| `export_markdown` | Document with MD | normal, disabled, hover, pressed |
| `format_csv` | CSV badge | normal, hover |
| `format_json` | JSON badge | normal, hover |
| `format_xml` | XML badge | normal, hover |

---

## 14. Reporting Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `report_new` | Document with star | normal, disabled, hover, pressed |
| `report_edit` | Document with pencil | normal, disabled, hover, pressed |
| `report_delete` | Document with X | normal, disabled, hover, pressed |
| `report_run` | Document with play | normal, disabled, hover, pressed |
| `report_schedule` | Document with clock | normal, disabled, hover, pressed |
| `report_share` | Document with share | normal, disabled, hover, pressed |
| `report_subscribe` | Document with bell | normal, disabled, hover, pressed |
| `report_chart_add` | Chart with plus | normal, disabled, hover, pressed |
| `report_table_add` | Table with plus | normal, disabled, hover, pressed |
| `report_dashboard` | Multiple charts | normal, disabled, hover, pressed |
| `report_kpi` | Target with number | normal, disabled, hover, pressed |
| `report_alert` | Bell with graph | normal, disabled, hover, pressed |
| `report_email` | Envelope with graph | normal, disabled, hover, pressed |

---

## 15. Navigation & UI Controls

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `nav_back` | Left arrow | normal, disabled, hover, pressed |
| `nav_forward` | Right arrow | normal, disabled, hover, pressed |
| `nav_up` | Up arrow | normal, disabled, hover, pressed |
| `nav_down` | Down arrow | normal, disabled, hover, pressed |
| `nav_home` | House | normal, disabled, hover, pressed |
| `nav_refresh` | Circular arrow | normal, disabled, hover, pressed, loading |
| `nav_close` | X | normal, hover, pressed |
| `nav_close_all` | Multiple X | normal, disabled, hover, pressed |
| `nav_maximize` | Square expand | normal, hover, pressed |
| `nav_minimize` | Line down | normal, hover, pressed |
| `nav_restore` | Overlapping squares | normal, hover, pressed |
| `nav_pin` | Pin/thumbtack | normal, hover, pressed, selected |
| `nav_pin_off` | Pin diagonal | normal, hover, pressed |
| `nav_collapse` | Chevron left/up | normal, hover, pressed |
| `nav_expand` | Chevron right/down | normal, hover, pressed |
| `nav_collapse_all` | Double chevron | normal, hover, pressed |
| `nav_expand_all` | Double chevron out | normal, hover, pressed |
| `nav_dropdown` | Small down arrow | normal, hover |
| `nav_menu` | Hamburger (3 lines) | normal, hover, pressed |
| `nav_splitter_h` | Horizontal resize | normal, hover |
| `nav_splitter_v` | Vertical resize | normal, hover |

---

## 16. Tree/List Icons

| Icon Name | Description | States |
|-----------|-------------|--------|
| `tree_folder` | Closed folder | normal, selected, open |
| `tree_folder_open` | Open folder | normal, selected |
| `tree_leaf` | Document/item | normal, selected |
| `tree_loading` | Spinner | normal (animated) |
| `tree_error` | Folder with X | normal |
| `tree_warning` | Folder with ! | normal |
| `tree_filter` | Folder with funnel | normal, selected |
| `tree_search` | Folder with magnifier | normal |
| `tree_favorite` | Folder with star | normal, selected |
| `tree_recent` | Folder with clock | normal, selected |

---

## 17. Notification Icons

| Icon Name | Description | States |
|-----------|-------------|--------|
| `notify_info` | Circle with i | normal |
| `notify_success` | Circle with check | normal |
| `notify_warning` | Triangle with ! | normal |
| `notify_error` | Circle with X | normal |
| `notify_question` | Circle with ? | normal |
| `notify_bell` | Bell | normal, has-unread |
| `notify_bell_off` | Bell with slash | normal |
| `toast_close` | Small X | normal, hover |

---

## 18. Git/Version Control Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `git_commit` | Circle with check | normal, disabled, hover, pressed |
| `git_branch` | Branch symbol | normal, disabled, hover, pressed |
| `git_merge` | Merging branches | normal, disabled, hover, pressed |
| `git_pull` | Down arrow branch | normal, disabled, hover, pressed |
| `git_push` | Up arrow branch | normal, disabled, hover, pressed |
| `git_fetch` | Circular arrow branch | normal, disabled, hover, pressed |
| `git_tag` | Tag label | normal, disabled, hover, pressed |
| `git_stash` | Box with branch | normal, disabled, hover, pressed |
| `git_cherry_pick` | Cherry with branch | normal, disabled, hover, pressed |
| `git_revert` | Curved back arrow | normal, disabled, hover, pressed |
| `git_conflict` | Exclamation on branch | normal, disabled, hover, pressed |
| `git_diff` | Split document | normal, disabled, hover, pressed |
| `git_blame` | Person on document | normal, disabled, hover, pressed |
| `git_history` | Clock on branch | normal, disabled, hover, pressed |
| `git_repository` | Database with branch | normal, disabled, hover, pressed |

---

## 19. AI/Assistant Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `ai_assistant` | Sparkle brain | normal, disabled, hover, pressed |
| `ai_suggest` | Lightbulb sparkle | normal, disabled, hover, pressed |
| `ai_generate` | Magic wand writing | normal, disabled, hover, pressed |
| `ai_optimize` | Speed gauge sparkle | normal, disabled, hover, pressed |
| `ai_explain` | Speech bubble sparkle | normal, disabled, hover, pressed |
| `ai_fix` | Wrench sparkle | normal, disabled, hover, pressed |
| `ai_chat` | Chat bubble sparkle | normal, disabled, hover, pressed |
| `ai_thinking` | Animated dots | normal (animated) |

---

## 20. Miscellaneous Icons

| Icon Name | Description | States Needed |
|-----------|-------------|---------------|
| `help_contents` | Book with ? | normal, disabled, hover, pressed |
| `help_context` | Cursor with ? | normal, disabled, hover, pressed |
| `help_about` | Info circle | normal, disabled, hover, pressed |
| `help_shortcuts` | Keyboard | normal, disabled, hover, pressed |
| `help_tutorial` | Graduation cap | normal, disabled, hover, pressed |
| `settings_general` | Gear | normal, disabled, hover, pressed, selected |
| `settings_editor` | Gear with code | normal, disabled, hover, pressed, selected |
| `settings_database` | Gear with cylinder | normal, disabled, hover, pressed, selected |
| `settings_appearance` | Gear with palette | normal, disabled, hover, pressed, selected |
| `settings_notifications` | Gear with bell | normal, disabled, hover, pressed, selected |
| `settings_shortcuts` | Gear with keyboard | normal, disabled, hover, pressed, selected |
| `tools_calculator` | Calculator | normal, disabled, hover, pressed |
| `tools_calendar` | Calendar | normal, disabled, hover, pressed |
| `tools_color_picker` | Dropper | normal, disabled, hover, pressed |
| `tools_terminal` | Terminal window | normal, disabled, hover, pressed |
| `clipboard_copy` | Two docs | normal |
| `clipboard_paste` | Clipboard | normal |
| `window_new` | Window with plus | normal, disabled, hover, pressed |
| `window_cascade` | Overlapping windows | normal, disabled, hover, pressed |
| `window_tile_h` | Horizontal tiles | normal, disabled, hover, pressed |
| `window_tile_v` | Vertical tiles | normal, disabled, hover, pressed |
| `fullscreen` | Four arrows out | normal, disabled, hover, pressed |
| `fullscreen_exit` | Four arrows in | normal, disabled, hover, pressed |
| `sidebar_left` | Left panel | normal, hover, pressed, selected |
| `sidebar_right` | Right panel | normal, hover, pressed, selected |
| `sidebar_bottom` | Bottom panel | normal, hover, pressed, selected |
| `dark_mode` | Moon | normal, hover, pressed, selected |
| `light_mode` | Sun | normal, hover, pressed, selected |
| `system_mode` | Sun-moon combo | normal, hover, pressed, selected |
| `language` | Globe | normal, disabled, hover, pressed |
| `update_available` | Download arrow | normal, hover |
| `sync_cloud` | Cloud circular arrows | normal, disabled, hover, pressed, syncing |
| `link` | Chain | normal, disabled, hover, pressed |
| `unlink` | Broken chain | normal, disabled, hover, pressed |
| `attachment` | Paperclip | normal, disabled, hover, pressed |
| `bookmark` | Star outline | normal, disabled, hover, pressed, selected |
| `bookmark_filled` | Star filled | normal, disabled, hover, pressed, selected |
| `star` | Star | normal, disabled, hover, pressed, selected |
| `flag` | Flag | normal, disabled, hover, pressed, selected |
| `tag` | Price tag | normal, disabled, hover, pressed |
| `filter_active` | Funnel filled | normal |
| `sort_indicator_asc` | Small up triangle | normal |
| `sort_indicator_desc` | Small down triangle | normal |
| `drag_handle` | Grid of dots | normal, hover |
| `drop_indicator` | Horizontal line | normal |
| `loading_spinner` | Circular progress | normal (animated) |
| `progress_indicator` | Horizontal bar | normal, animated |
| `resize_handle` | Diagonal lines | normal |

---

## Icon Design Guidelines

### Style Specifications

```
Style: Modern flat with subtle depth
Size variants: 16x16, 24x24, 32x32, 48x48
Format: SVG (scalable)
Stroke width: 1.5px (16px), 2px (24px+)
Corner radius: 2px (small), 4px (large elements)
```

### Color Palette

| Usage | Color | Hex |
|-------|-------|-----|
| Primary | Robin Orange | #FF6B35 |
| Secondary | Database Blue | #2E86AB |
| Success | Green | #28A745 |
| Warning | Amber | #FFC107 |
| Error | Red | #DC3545 |
| Info | Blue | #17A2B8 |
| Neutral (normal) | Dark Gray | #495057 |
| Neutral (disabled) | Light Gray | #ADB5BD |
| Background (selected) | Light Blue | #E3F2FD |

### State Visual Treatments

**Normal:** Full color (#495057), 100% opacity
**Disabled:** Grayscale conversion, 40% opacity, no shadow
**Hover:** 10% brightness increase, subtle drop shadow (0 2px 4px rgba(0,0,0,0.1))
**Pressed:** 15% darker, inset shadow
**Selected:** White icon on primary color background (#2E86AB)
**Focused:** 2px outline in focus color (#FF6B35)

---

## File Naming Convention

```
[category]_[name]_[size]_[state].svg

Examples:
- action_save_16_normal.svg
- action_save_16_disabled.svg
- action_save_16_hover.svg
- db_table_24_normal.svg
- db_table_24_selected.svg
```

## Directory Structure

```
assets/icons/
├── 16x16/           # Small toolbar/menu icons
├── 24x24/           # Standard toolbar icons
├── 32x32/           # Large toolbar/buttons
├── 48x48/           # Dialog icons
├── 64x64/           # Welcome page/feature icons
├── 128x128/         # Splash screen/about dialog
├── 256x256/         # High-res/application icons
├── svg/             # Source SVG files
│   ├── actions/
│   ├── database/
│   ├── diagrams/
│   ├── navigation/
│   └── status/
└── icon_specification.md  # This file
```

---

## Total Icon Count Estimate

| Category | Base Icons | States | Total Variants |
|----------|-----------|--------|----------------|
| Application | 5 | 2 | 10 |
| File Operations | 12 | 4 | 48 |
| Edit Operations | 11 | 4 | 44 |
| Connection | 10 | 5 | 50 |
| Database Objects | 25 | 3 | 75 |
| SQL Editor | 14 | 4 | 56 |
| Data Grid | 25 | 4 | 100 |
| Diagram Tools | 20 | 4 | 80 |
| Diagram Entities | 8 | 3 | 24 |
| Diagram Relations | 6 | 5 | 30 |
| Diagram Layout | 14 | 4 | 56 |
| Schema Compare | 12 | 4 | 48 |
| Backup/Restore | 11 | 4 | 44 |
| User/Security | 15 | 4 | 60 |
| Monitoring | 14 | 4 | 56 |
| Import/Export | 15 | 4 | 60 |
| Reporting | 12 | 4 | 48 |
| Navigation | 22 | 4 | 88 |
| Tree/List | 10 | 3 | 30 |
| Notifications | 9 | 2 | 18 |
| Git/VCS | 14 | 4 | 56 |
| AI/Assistant | 8 | 4 | 32 |
| Miscellaneous | 45 | 4 | 180 |
| **TOTAL** | **347** | **~4 avg** | **~1,388 variants** |

**Priority Tiers:**
- **P0 (Must have):** ~80 icons - Core navigation, database objects, basic actions
- **P1 (Should have):** ~120 icons - Full toolbar sets, status indicators
- **P2 (Nice to have):** ~147 icons - Advanced features, secondary actions
