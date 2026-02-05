/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_EXPORT_MANAGER_H
#define SCRATCHROBIN_DIAGRAM_EXPORT_MANAGER_H

#include "../ui/diagram_model.h"

#include <cstdint>
#include <string>
#include <vector>

class wxDC;
class wxWindow;

namespace scratchrobin {
namespace diagram {

// Export formats
enum class ExportFormat {
    PNG,
    JPEG,
    BMP,
    SVG,
    PDF
};

// Export options
struct ExportOptions {
    ExportFormat format = ExportFormat::PNG;
    int dpi = 96;                       // Resolution for raster formats
    double scale = 1.0;                 // Scale factor
    bool transparent_background = false; // For PNG
    bool include_grid = false;          // Include grid in export
    int quality = 90;                   // For JPEG (0-100)
    
    // Page settings for PDF
    std::string paper_size = "A4";      // A4, Letter, etc.
    bool landscape = false;
    bool fit_to_page = true;
    double margin = 20.0;               // mm
};

// Export result
struct ExportResult {
    bool success = false;
    std::string error_message;
    int width = 0;
    int height = 0;
    std::string file_path;
};

// Export manager
class ExportManager {
public:
    // Export diagram to file
    static ExportResult ExportToFile(const DiagramModel& model,
                                      const std::string& file_path,
                                      const ExportOptions& options);
    
    // Export diagram to bitmap (for clipboard/copy)
    static std::vector<uint8_t> ExportToBitmap(const DiagramModel& model,
                                                const ExportOptions& options,
                                                int& out_width,
                                                int& out_height);
    
    // Export to SVG string
    static std::string ExportToSvg(const DiagramModel& model,
                                    const ExportOptions& options);
    
    // Get supported export formats
    static std::vector<std::pair<ExportFormat, std::string>> GetSupportedFormats();
    
    // Get file extension for format
    static std::string GetFileExtension(ExportFormat format);
    
    // Get format from file extension
    static ExportFormat GetFormatFromExtension(const std::string& ext);

private:
    // Raster export (PNG, JPEG, BMP)
    static ExportResult ExportRaster(const DiagramModel& model,
                                      const std::string& file_path,
                                      const ExportOptions& options);
    
    // SVG export
    static ExportResult ExportSvg(const DiagramModel& model,
                                   const std::string& file_path,
                                   const ExportOptions& options);
    
    // PDF export
    static ExportResult ExportPdf(const DiagramModel& model,
                                   const std::string& file_path,
                                   const ExportOptions& options);
    
    // Render diagram to DC
    static void RenderToDc(wxDC& dc, const DiagramModel& model,
                           const ExportOptions& options);
    
    // SVG helper functions
    static std::string SvgHeader(int width, int height);
    static std::string SvgFooter();
    static std::string SvgRect(double x, double y, double w, double h,
                                const std::string& fill, const std::string& stroke);
    static std::string SvgRoundedRect(double x, double y, double w, double h, double r,
                                       const std::string& fill, const std::string& stroke);
    static std::string SvgRectDashed(double x, double y, double w, double h,
                                      const std::string& fill, const std::string& stroke);
    static std::string SvgEllipse(double x, double y, double w, double h,
                                   const std::string& fill, const std::string& stroke);
    static std::string SvgText(double x, double y, const std::string& text,
                                const std::string& font_family, int font_size);
    static std::string SvgLine(double x1, double y1, double x2, double y2,
                                const std::string& stroke, int width);
    static std::string SvgArrow(double x1, double y1, double x2, double y2,
                                 const std::string& stroke);
};

// Import from various formats (for future expansion)
class ImportManager {
public:
    // Import from SQL DDL (CREATE TABLE statements)
    static bool ImportFromDdl(DiagramModel* model, const std::string& ddl);
    
    // Import from other ERD formats (for future)
    // static bool ImportFromDbml(DiagramModel* model, const std::string& dbml);
    // static bool ImportFromERDPlus(DiagramModel* model, const std::string& json);
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_EXPORT_MANAGER_H
