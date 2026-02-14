# Diagram Style System

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines a unified styling system shared by all diagram types (Silverston, ERD, Whiteboard, Mind Map, Dataflow). This ensures consistent look, feel, and export behavior across the application.

---

## 1. Global Style Tokens

All diagrams use a shared set of tokens. Each diagram may override these, but defaults must be consistent.

### 1.1 Core Tokens

- `font_family`
- `font_size_base`
- `line_width`
- `border_radius`
- `icon_size_min`
- `icon_size_max`

### 1.2 Color Tokens

- `color_background`
- `color_surface`
- `color_text`
- `color_accent`
- `color_warning`
- `color_success`
- `color_error`

### 1.3 Line Tokens

- `line_default`
- `line_dependency`
- `line_relationship`
- `line_highlight`

---

## 2. Template Inheritance

- Global tokens → Diagram type defaults → Object template → Object overrides.
- Object overrides apply only to that instance.

---

## 3. Selection and Interaction States

- Hover: +10% brightness
- Selection: 2px accent border + handles
- Disabled: 50% opacity

---

## 4. Export Profiles

Export profiles define output styles for print/PDF/web:

- **Executive**: minimal detail, bold titles
- **Design Review**: balanced detail
- **Implementation**: full detail

---

## 5. Configuration Example

```yaml
style_tokens:
  font_family: "Inter"
  font_size_base: 12
  line_width: 2
  icon_size_min: 16
  icon_size_max: 48
  color_background: "#FFFFFF"
  color_text: "#111111"
```

