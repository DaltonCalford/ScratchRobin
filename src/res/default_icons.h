/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

// Default SVG icons for ScratchRobin
// These are placeholders until custom SVG icons are created for each object type
// All icons are 16x16 viewBox and will scale to any size

namespace scratchrobin::res {

// Default database object icon (generic cylinder/database shape)
inline constexpr const char DEFAULT_OBJECT_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><ellipse cx="8" cy="3.5" rx="6" ry="2" fill="#6c757d" stroke="#495057" stroke-width="1"/><path d="M2 3.5v9c0 1.1 2.7 2 6 2s6-.9 6-2v-9" fill="#6c757d" stroke="#495057" stroke-width="1"/><line x1="2" y1="8" x2="14" y2="8" stroke="#495057" stroke-width="0.5" opacity="0.5"/></svg>)svg";

// Root/Server icon (server tower)
inline constexpr const char ROOT_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="1" width="12" height="14" rx="1" fill="#495057" stroke="#212529" stroke-width="1"/><circle cx="5" cy="4" r="1" fill="#28a745"/><circle cx="5" cy="8" r="1" fill="#28a745"/><circle cx="5" cy="12" r="1" fill="#28a745"/><line x1="8" y1="4" x2="12" y2="4" stroke="#adb5bd" stroke-width="1"/><line x1="8" y1="8" x2="12" y2="8" stroke="#adb5bd" stroke-width="1"/><line x1="8" y1="12" x2="12" y2="12" stroke="#adb5bd" stroke-width="1"/></svg>)svg";

// Database icon (connected cylinder)
inline constexpr const char DATABASE_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><ellipse cx="8" cy="3.5" rx="6" ry="2" fill="#28a745" stroke="#1e7e34" stroke-width="1"/><path d="M2 3.5v9c0 1.1 2.7 2 6 2s6-.9 6-2v-9" fill="#28a745" stroke="#1e7e34" stroke-width="1" opacity="0.8"/><line x1="2" y1="8" x2="14" y2="8" stroke="#1e7e34" stroke-width="0.5"/></svg>)svg";

// Table icon (grid)
inline constexpr const char TABLE_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="1" y="2" width="14" height="12" rx="1" fill="#4a90d9" stroke="#2e5c8a" stroke-width="1"/><line x1="1" y1="6" x2="15" y2="6" stroke="#2e5c8a" stroke-width="1"/><line x1="6" y1="2" x2="6" y2="14" stroke="#2e5c8a" stroke-width="1"/><line x1="11" y1="2" x2="11" y2="14" stroke="#2e5c8a" stroke-width="1"/></svg>)svg";

// Column icon (vertical bar with type indicator)
inline constexpr const char COLUMN_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="5" y="1" width="6" height="14" rx="1" fill="#adb5bd" stroke="#6c757d" stroke-width="1"/><line x1="8" y1="4" x2="8" y2="12" stroke="#495057" stroke-width="1.5"/></svg>)svg";

// Primary Key column icon (column with key)
inline constexpr const char COLUMN_PK_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="5" y="1" width="6" height="14" rx="1" fill="#adb5bd" stroke="#6c757d" stroke-width="1"/><circle cx="8" cy="5" r="2.5" fill="#ffc107" stroke="#d39e00" stroke-width="1"/><path d="M8 7.5v5" stroke="#495057" stroke-width="1.5" stroke-linecap="round"/></svg>)svg";

// Foreign Key column icon (column with link)
inline constexpr const char COLUMN_FK_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="5" y="1" width="6" height="14" rx="1" fill="#adb5bd" stroke="#6c757d" stroke-width="1"/><path d="M4 8h3v-2l3 3-3 3v-2h-3" fill="none" stroke="#17a2b8" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>)svg";

// View icon (document with eye)
inline constexpr const char VIEW_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="1" width="12" height="14" rx="1" fill="#6f42c1" stroke="#4a2c85" stroke-width="1"/><ellipse cx="8" cy="8" rx="3" ry="2.5" fill="#e9ecef" stroke="#4a2c85" stroke-width="1"/><circle cx="8" cy="8" r="1.5" fill="#4a2c85"/></svg>)svg";

// Procedure/Function icon (gear/document)
inline constexpr const char PROCEDURE_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="3" y="1" width="10" height="14" rx="1" fill="#fd7e14" stroke="#c45a08" stroke-width="1"/><circle cx="8" cy="8" r="2.5" fill="#fff" stroke="#c45a08" stroke-width="1"/><path d="M8 4v1.5M8 10.5V12M4 8h1.5M10.5 8H12M5.05 5.05l1.06 1.06M9.9 9.9l1.06 1.06M5.05 10.95l1.06-1.06M9.9 6.1l1.06-1.06" stroke="#c45a08" stroke-width="1" stroke-linecap="round"/></svg>)svg";

// Function icon (f(x))
inline constexpr const char FUNCTION_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16"height="16"><rect x="2" y="2" width="12" height="12" rx="1" fill="#e83e8c" stroke="#b01a5c" stroke-width="1"/><text x="8" y="11.5" font-family="Arial, sans-serif" font-size="7" font-weight="bold" fill="#fff" text-anchor="middle">f(x)</text></svg>)svg";

// Trigger icon (lightning bolt)
inline constexpr const char TRIGGER_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><polygon points="9,1 4,9 8,9 7,15 12,7 8,7" fill="#ffc107" stroke="#d39e00" stroke-width="1" stroke-linejoin="round"/></svg>)svg";

// Index icon (sorted list with arrow)
inline constexpr const char INDEX_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="2" width="8" height="2" fill="#17a2b8"/><rect x="2" y="6" width="6" height="2" fill="#17a2b8"/><rect x="2" y="10" width="10" height="2" fill="#17a2b8"/><path d="M12 3v8M12 3l-2 2M12 3l2 2" stroke="#dc3545" stroke-width="1.5" fill="none" stroke-linecap="round" stroke-linejoin="round"/></svg>)svg";

// Domain icon (constrained box)
inline constexpr const char DOMAIN_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="3" width="12" height="10" rx="1" fill="#20c997" stroke="#0f8c6d" stroke-width="1"/><rect x="4" y="5" width="8" height="6" fill="none" stroke="#fff" stroke-width="1" stroke-dasharray="2,1"/></svg>)svg";

// Generator/Sequence icon (counter)
inline constexpr const char GENERATOR_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="1" y="3" width="14" height="10" rx="1" fill="#6c757d" stroke="#495057" stroke-width="1"/><text x="8" y="11" font-family="Arial, sans-serif" font-size="6" font-weight="bold" fill="#fff" text-anchor="middle">123</text></svg>)svg";

// Package icon (box with modules)
inline constexpr const char PACKAGE_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="3" width="12" height="10" rx="1" fill="#6610f2" stroke="#3f0ca4" stroke-width="1"/><rect x="4" y="5" width="3" height="2.5" fill="#fff"/><rect x="9" y="5" width="3" height="2.5" fill="#fff"/><rect x="4" y="8.5" width="3" height="2.5" fill="#fff"/><rect x="9" y="8.5" width="3" height="2.5" fill="#fff"/></svg>)svg";

// User icon (person silhouette)
inline constexpr const char USER_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><circle cx="8" cy="5" r="3" fill="#17a2b8" stroke="#0f6674" stroke-width="1"/><path d="M3 14c0-2.8 2.2-5 5-5s5 2.2 5 5" fill="#17a2b8" stroke="#0f6674" stroke-width="1"/></svg>)svg";

// Role icon (shield with key)
inline constexpr const char ROLE_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><path d="M8 1L2 4v4c0 4 2.5 6.5 6 7 3.5-.5 6-3 6-7V4L8 1z" fill="#dc3545" stroke="#a71d2a" stroke-width="1"/><circle cx="8" cy="7" r="1.5" fill="#fff"/><path d="M8 8.5v3" stroke="#fff" stroke-width="1.5" stroke-linecap="round"/></svg>)svg";

// Exception icon (warning triangle)
inline constexpr const char EXCEPTION_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><path d="M8 1L1 14h14L8 1z" fill="#ffc107" stroke="#d39e00" stroke-width="1" stroke-linejoin="round"/><line x1="8" y1="6" x2="8" y2="9" stroke="#212529" stroke-width="1.5" stroke-linecap="round"/><circle cx="8" cy="11.5" r="0.8" fill="#212529"/></svg>)svg";

// Folder icon (open folder)
inline constexpr const char FOLDER_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><path d="M1 3c0-1.1.9-2 2-2h3l2 2h5c1.1 0 2 .9 2 2v8c0 1.1-.9 2-2 2H3c-1.1 0-2-.9-2-2V3z" fill="#ffc107" stroke="#d39e00" stroke-width="1"/></svg>)svg";

// Folder open icon (expanded folder)
inline constexpr const char FOLDER_OPEN_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><path d="M1 3c0-1.1.9-2 2-2h3l2 2h5c1.1 0 2 .9 2 2v1H3c-1.1 0-2 .9-2 2v6c0-1.1.9-2 2-2h10l2-6H3V3z" fill="#ffc107" stroke="#d39e00" stroke-width="1"/></svg>)svg";

// Input parameter icon (arrow in)
inline constexpr const char INPUT_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="5" width="8" height="6" rx="1" fill="#adb5bd" stroke="#6c757d" stroke-width="1"/><path d="M14 8H8M8 8l3-3M8 8l3 3" stroke="#28a745" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>)svg";

// Output parameter icon (arrow out)
inline constexpr const char OUTPUT_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="6" y="5" width="8" height="6" rx="1" fill="#adb5bd" stroke="#6c757d" stroke-width="1"/><path d="M2 8h6M8 8l-3-3M8 8l-3 3" stroke="#dc3545" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>)svg";

// Character Set icon (A with encoding)
inline constexpr const char CHARSET_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="1" y="2" width="14" height="12" rx="1" fill="#6c757d" stroke="#495057" stroke-width="1"/><text x="8" y="11" font-family="Arial, sans-serif" font-size="8" font-weight="bold" fill="#fff" text-anchor="middle">A</text></svg>)svg";

// Collation icon (sorted A-Z)
inline constexpr const char COLLATION_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><text x="4" y="7" font-family="Arial, sans-serif" font-size="5" fill="#495057">A</text><text x="4" y="13" font-family="Arial, sans-serif" font-size="5" fill="#495057">Z</text><path d="M10 4v8M10 4l-2 2M10 4l2 2M10 12l-2-2M10 12l2-2" stroke="#dc3545" stroke-width="1" stroke-linecap="round" stroke-linejoin="round"/></svg>)svg";

// UDF icon (puzzle piece)
inline constexpr const char UDF_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><path d="M4 1h3v2.5a1.5 1.5 0 013 0V1h3v3a1 1 0 01-1 1h-.5a.5.5 0 000 1H12a1 1 0 011 1v3h-2.5a1.5 1.5 0 010-3H14V1h-3v2.5a1.5 1.5 0 01-3 0V1H5v3a1 1 0 001 1h.5a.5.5 0 000 1H6a1 1 0 00-1 1v3h2.5a1.5 1.5 0 010 3H4v3h3v-2.5a1.5 1.5 0 013 0V15h3" fill="#6c757d" stroke="#495057" stroke-width="1" stroke-linejoin="round"/></svg>)svg";

// System object variants (grayed out versions)
inline constexpr const char SYSTEM_TABLE_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="1" y="2" width="14" height="12" rx="1" fill="#adb5bd" stroke="#6c757d" stroke-width="1"/><line x1="1" y1="6" x2="15" y2="6" stroke="#6c757d" stroke-width="1"/><line x1="6" y1="2" x2="6" y2="14" stroke="#6c757d" stroke-width="1"/><line x1="11" y1="2" x2="11" y2="14" stroke="#6c757d" stroke-width="1"/></svg>)svg";

inline constexpr const char SYSTEM_DOMAIN_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="3" width="12" height="10" rx="1" fill="#adb5bd" stroke="#6c757d" stroke-width="1"/><rect x="4" y="5" width="8" height="6" fill="none" stroke="#dee2e6" stroke-width="1" stroke-dasharray="2,1"/></svg>)svg";

// Global Temporary Table icon (table with clock)
inline constexpr const char GTT_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="1" y="2" width="14" height="12" rx="1" fill="#4a90d9" stroke="#2e5c8a" stroke-width="1"/><line x1="1" y1="6" x2="15" y2="6" stroke="#2e5c8a" stroke-width="1"/><line x1="6" y1="2" x2="6" y2="14" stroke="#2e5c8a" stroke-width="1"/><circle cx="12" cy="10" r="3" fill="#ffc107" stroke="#d39e00" stroke-width="1"/><path d="M12 8v2l1.5 1" stroke="#212529" stroke-width="1" stroke-linecap="round"/></svg>)svg";

// DML Trigger icon (table with lightning)
inline constexpr const char DML_TRIGGER_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="1" y="2" width="14" height="12" rx="1" fill="#4a90d9" stroke="#2e5c8a" stroke-width="1" opacity="0.3"/><polygon points="12,3 8,8 11,8 10,13 14,8 11,8" fill="#ffc107" stroke="#d39e00" stroke-width="1" stroke-linejoin="round"/></svg>)svg";

// DB Trigger icon (database with lightning)
inline constexpr const char DB_TRIGGER_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><ellipse cx="7" cy="3" rx="5" ry="1.5" fill="#28a745" stroke="#1e7e34" stroke-width="1" opacity="0.5"/><path d="M2 3v7c0 .9 2.2 1.7 5 1.7s5-.8 5-1.7V3" fill="#28a745" stroke="#1e7e34" stroke-width="1" opacity="0.5"/><polygon points="13,6 10,10 12,10 11,14 15,10 13,10" fill="#ffc107" stroke="#d39e00" stroke-width="1" stroke-linejoin="round"/></svg>)svg";

// DDL Trigger icon (schema/wrench with lightning)
inline constexpr const char DDL_TRIGGER_SVG[] = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"><rect x="2" y="2" width="8" height="8" rx="1" fill="#6c757d" stroke="#495057" stroke-width="1"/><path d="M4 6h4M6 4v4" stroke="#fff" stroke-width="1.5" stroke-linecap="round"/><polygon points="13,6 10,10 12,10 11,14 15,10 13,10" fill="#ffc107" stroke="#d39e00" stroke-width="1" stroke-linejoin="round"/></svg>)svg";

}  // namespace scratchrobin::res
