/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "export_manager.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

namespace scratchrobin {
namespace diagram {

// Supported formats
std::vector<std::pair<ExportFormat, std::string>> ExportManager::GetSupportedFormats() {
    return {
        {ExportFormat::PNG, "PNG Image"},
        {ExportFormat::JPEG, "JPEG Image"},
        {ExportFormat::BMP, "BMP Image"},
        {ExportFormat::SVG, "SVG Vector"},
        {ExportFormat::PDF, "PDF Document"}
    };
}

std::string ExportManager::GetFileExtension(ExportFormat format) {
    switch (format) {
        case ExportFormat::PNG: return "png";
        case ExportFormat::JPEG: return "jpg";
        case ExportFormat::BMP: return "bmp";
        case ExportFormat::SVG: return "svg";
        case ExportFormat::PDF: return "pdf";
        default: return "png";
    }
}

ExportFormat ExportManager::GetFormatFromExtension(const std::string& ext) {
    std::string lower = ext;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "png") return ExportFormat::PNG;
    if (lower == "jpg" || lower == "jpeg") return ExportFormat::JPEG;
    if (lower == "bmp") return ExportFormat::BMP;
    if (lower == "svg") return ExportFormat::SVG;
    if (lower == "pdf") return ExportFormat::PDF;
    return ExportFormat::PNG;
}

ExportResult ExportManager::ExportToFile(const DiagramModel& model,
                                          const std::string& file_path,
                                          const ExportOptions& options) {
    switch (options.format) {
        case ExportFormat::PNG:
        case ExportFormat::JPEG:
        case ExportFormat::BMP:
            return ExportRaster(model, file_path, options);
        case ExportFormat::SVG:
            return ExportSvg(model, file_path, options);
        case ExportFormat::PDF:
            return ExportPdf(model, file_path, options);
        default:
            return ExportRaster(model, file_path, options);
    }
}

ExportResult ExportManager::ExportRaster(const DiagramModel& model,
                                          const std::string& file_path,
                                          const ExportOptions& options) {
    ExportResult result;
    result.file_path = file_path;
    
    // Calculate bounds
    if (model.nodes().empty()) {
        result.error_message = "No nodes to export";
        return result;
    }
    
    double min_x = model.nodes()[0].x;
    double min_y = model.nodes()[0].y;
    double max_x = min_x + model.nodes()[0].width;
    double max_y = min_y + model.nodes()[0].height;
    
    for (const auto& node : model.nodes()) {
        min_x = std::min(min_x, node.x);
        min_y = std::min(min_y, node.y);
        max_x = std::max(max_x, node.x + node.width);
        max_y = std::max(max_y, node.y + node.height);
    }
    
    int width = static_cast<int>((max_x - min_x + 100) * options.scale);
    int height = static_cast<int>((max_y - min_y + 100) * options.scale);
    
    result.width = width;
    result.height = height;
    
    // Create bitmap
    wxBitmap bitmap(width, height);
    wxMemoryDC dc(bitmap);
    
    // Background
    if (options.transparent_background && options.format == ExportFormat::PNG) {
        dc.SetBackground(*wxTRANSPARENT_BRUSH);
    } else {
        dc.SetBackground(wxBrush(wxColour(255, 255, 255)));
    }
    dc.Clear();
    
    // Offset to center content
    dc.SetDeviceOrigin(static_cast<int>(-min_x + 50), static_cast<int>(-min_y + 50));
    dc.SetUserScale(options.scale, options.scale);
    
    // Render diagram
    RenderToDc(dc, model, options);
    
    dc.SelectObject(wxNullBitmap);
    
    // Save to file
    wxImage image = bitmap.ConvertToImage();
    
    if (options.format == ExportFormat::PNG) {
        if (options.transparent_background) {
            image.InitAlpha();
        }
        result.success = image.SaveFile(wxString::FromUTF8(file_path), wxBITMAP_TYPE_PNG);
    } else if (options.format == ExportFormat::JPEG) {
        image.SetOption(wxIMAGE_OPTION_QUALITY, options.quality);
        result.success = image.SaveFile(wxString::FromUTF8(file_path), wxBITMAP_TYPE_JPEG);
    } else if (options.format == ExportFormat::BMP) {
        result.success = image.SaveFile(wxString::FromUTF8(file_path), wxBITMAP_TYPE_BMP);
    }
    
    if (!result.success) {
        result.error_message = "Failed to save image file";
    }
    
    return result;
}

ExportResult ExportManager::ExportSvg(const DiagramModel& model,
                                       const std::string& file_path,
                                       const ExportOptions& options) {
    ExportResult result;
    result.file_path = file_path;
    
    std::string svg = ExportToSvg(model, options);
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
        result.error_message = "Failed to open file for writing";
        return result;
    }
    
    file << svg;
    file.close();
    
    result.success = true;
    return result;
}

std::string ExportManager::ExportToSvg(const DiagramModel& model,
                                        const ExportOptions& options) {
    // Calculate bounds
    double min_x = 0, min_y = 0, max_x = 800, max_y = 600;
    if (!model.nodes().empty()) {
        min_x = model.nodes()[0].x;
        min_y = model.nodes()[0].y;
        max_x = min_x + model.nodes()[0].width;
        max_y = min_y + model.nodes()[0].height;
        
        for (const auto& node : model.nodes()) {
            min_x = std::min(min_x, node.x);
            min_y = std::min(min_y, node.y);
            max_x = std::max(max_x, node.x + node.width);
            max_y = std::max(max_y, node.y + node.height);
        }
    }
    
    int width = static_cast<int>((max_x - min_x + 100) * options.scale);
    int height = static_cast<int>((max_y - min_y + 100) * options.scale);
    
    std::stringstream svg;
    svg << SvgHeader(width, height);
    
    // Background
    svg << "  <rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";
    
    // Render edges first (behind nodes)
    for (const auto& edge : model.edges()) {
        // Find source and target nodes
        auto source_it = std::find_if(model.nodes().begin(), model.nodes().end(),
                                      [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
        auto target_it = std::find_if(model.nodes().begin(), model.nodes().end(),
                                      [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
        
        if (source_it == model.nodes().end() || target_it == model.nodes().end()) {
            continue;
        }
        
        double x1 = (source_it->x + source_it->width / 2 - min_x + 50) * options.scale;
        double y1 = (source_it->y + source_it->height / 2 - min_y + 50) * options.scale;
        double x2 = (target_it->x + target_it->width / 2 - min_x + 50) * options.scale;
        double y2 = (target_it->y + target_it->height / 2 - min_y + 50) * options.scale;
        
        svg << SvgLine(x1, y1, x2, y2, "#666666", 2);
        if (model.type() == DiagramType::Silverston ||
            model.type() == DiagramType::DataFlow ||
            model.type() == DiagramType::MindMap) {
            svg << SvgArrow(x1, y1, x2, y2, "#666666");
        }
        
        // Cardinality labels
        if (!edge.label.empty()) {
            svg << SvgText((x1 + x2) / 2, (y1 + y2) / 2 - 10, edge.label, "Arial", 10);
        }
    }
    
    // Render nodes
    for (const auto& node : model.nodes()) {
        double x = (node.x - min_x + 50) * options.scale;
        double y = (node.y - min_y + 50) * options.scale;
        double w = node.width * options.scale;
        double h = node.height * options.scale;
        
        if (model.type() == DiagramType::MindMap) {
            svg << SvgEllipse(x, y, w, h, "#e6f0ff", "#335577");
            svg << SvgText(x + w / 2 - 20, y + h / 2 + 4, node.name, "Arial", 12);
        } else if (model.type() == DiagramType::DataFlow) {
            if (node.type == "Process") {
                svg << SvgRoundedRect(x, y, w, h, 12, "#e8f2ff", "#335577");
            } else if (node.type == "Data Store") {
                svg << SvgRect(x, y, w, h, "#f7f7f7", "#444444");
                svg << SvgLine(x + 6, y, x + 6, y + h, "#444444", 2);
                svg << SvgLine(x + w - 6, y, x + w - 6, y + h, "#444444", 2);
            } else {
                svg << SvgRect(x, y, w, h, "#f7f7f7", "#444444");
            }
            svg << SvgText(x + 6, y + 18, node.name, "Arial", 12);
        } else if (model.type() == DiagramType::Whiteboard) {
            if (node.type == "Note") {
                svg << SvgRect(x, y, w, h, "#f2e698", "#8a7a2f");
            } else if (node.type == "Sketch") {
                svg << SvgRectDashed(x, y, w, h, "#f9f9f9", "#777777");
            } else {
                svg << SvgRect(x, y, w, h, "#f0f0f0", "#333333");
            }
            svg << SvgText(x + 6, y + 18, node.name, "Arial", 12);
        } else if (model.type() == DiagramType::Silverston) {
            svg << SvgRect(x, y, w, h, "#2f353a", "#8f9aa3");
            svg << SvgText(x + 6, y + 18, node.name, "Arial", 12);
            svg << SvgText(x + 6, y + 34, node.type, "Arial", 10);
        } else {
            // ERD default
            svg << SvgRect(x, y, w, h, "#f0f0f0", "#333333");
            svg << "  <rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w 
                << "\" height=\"25\" fill=\"#e0e0e0\" stroke=\"#333333\"/>\n";
            svg << SvgText(x + 5, y + 17, node.name, "Arial", 12);
            double attr_y = y + 35;
            for (const auto& attr : node.attributes) {
                std::string text = attr.name + " : " + attr.data_type;
                svg << SvgText(x + 5, attr_y, text, "Arial", 10);
                attr_y += 14;
            }
        }
    }
    
    svg << SvgFooter();
    return svg.str();
}

ExportResult ExportManager::ExportPdf(const DiagramModel& model,
                                       const std::string& file_path,
                                       const ExportOptions& options) {
    ExportResult result;
    result.file_path = file_path;
    
    // For now, we'll generate a simple PDF-like structure
    // In production, you'd use a PDF library like libharu or similar
    
    // As a placeholder, export to SVG and note that PDF export would use a library
    std::string svg_content = ExportToSvg(model, options);
    
    // Simple approach: create an HTML wrapper that can be printed to PDF
    std::stringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html><head><title>ERD Export</title></head><body>\n";
    html << "<h1>Database Schema Diagram</h1>\n";
    html << "<p>Generated by ScratchRobin</p>\n";
    html << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"600\">\n";
    
    // Strip SVG header/footer and embed content
    size_t content_start = svg_content.find("<rect");
    size_t content_end = svg_content.rfind("</svg>");
    if (content_start != std::string::npos && content_end != std::string::npos) {
        html << svg_content.substr(content_start, content_end - content_start);
    }
    
    html << "</svg></body></html>\n";
    
    std::string html_path = file_path.substr(0, file_path.find_last_of('.')) + ".html";
    std::ofstream file(html_path);
    if (file.is_open()) {
        file << html.str();
        file.close();
        result.success = true;
        result.error_message = "PDF export generated as HTML (print to PDF in browser)";
    } else {
        result.error_message = "Failed to create export file";
    }
    
    return result;
}

void ExportManager::RenderToDc(wxDC& dc, const DiagramModel& model,
                                const ExportOptions& options) {
    // Simple rendering - in production this would use the same rendering as DiagramCanvas
    dc.SetPen(wxPen(wxColour(100, 100, 100), 2));
    dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
    dc.SetTextForeground(wxColour(0, 0, 0));
    
    // Draw edges
    for (const auto& edge : model.edges()) {
        auto source_it = std::find_if(model.nodes().begin(), model.nodes().end(),
                                      [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
        auto target_it = std::find_if(model.nodes().begin(), model.nodes().end(),
                                      [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
        
        if (source_it != model.nodes().end() && target_it != model.nodes().end()) {
            int x1 = static_cast<int>(source_it->x + source_it->width / 2);
            int y1 = static_cast<int>(source_it->y + source_it->height / 2);
            int x2 = static_cast<int>(target_it->x + target_it->width / 2);
            int y2 = static_cast<int>(target_it->y + target_it->height / 2);
            dc.DrawLine(x1, y1, x2, y2);
        }
    }
    
    // Draw nodes
    for (const auto& node : model.nodes()) {
        int x = static_cast<int>(node.x);
        int y = static_cast<int>(node.y);
        int w = static_cast<int>(node.width);
        int h = static_cast<int>(node.height);

        if (model.type() == DiagramType::MindMap) {
            dc.DrawEllipse(x, y, w, h);
            dc.DrawText(node.name, x + 8, y + 8);
        } else if (model.type() == DiagramType::DataFlow) {
            if (node.type == "Process") {
                dc.DrawRoundedRectangle(x, y, w, h, 8);
            } else if (node.type == "Data Store") {
                dc.DrawRectangle(x, y, w, h);
                dc.DrawLine(x + 6, y, x + 6, y + h);
                dc.DrawLine(x + w - 6, y, x + w - 6, y + h);
            } else {
                dc.DrawRectangle(x, y, w, h);
            }
            dc.DrawText(node.name, x + 5, y + 5);
        } else if (model.type() == DiagramType::Whiteboard) {
            if (node.type == "Note") {
                dc.SetBrush(wxBrush(wxColour(242, 230, 152)));
            }
            dc.DrawRectangle(x, y, w, h);
            dc.DrawText(node.name, x + 5, y + 5);
        } else {
            dc.DrawRectangle(x, y, w, h);
            dc.DrawText(node.name, x + 5, y + 5);
        }
    }
}

// SVG Helper functions
std::string ExportManager::SvgHeader(int width, int height) {
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width 
       << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " " << height << "\">\n";
    return ss.str();
}

std::string ExportManager::SvgFooter() {
    return "</svg>\n";
}

std::string ExportManager::SvgRect(double x, double y, double w, double h,
                                    const std::string& fill, const std::string& stroke) {
    std::stringstream ss;
    ss << "  <rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w 
       << "\" height=\"" << h << "\" fill=\"" << fill << "\" stroke=\"" 
       << stroke << "\" stroke-width=\"1\"/>\n";
    return ss.str();
}

std::string ExportManager::SvgRoundedRect(double x, double y, double w, double h, double r,
                                           const std::string& fill, const std::string& stroke) {
    std::stringstream ss;
    ss << "  <rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w
       << "\" height=\"" << h << "\" rx=\"" << r << "\" ry=\"" << r
       << "\" fill=\"" << fill << "\" stroke=\"" << stroke << "\" stroke-width=\"1\"/>\n";
    return ss.str();
}

std::string ExportManager::SvgRectDashed(double x, double y, double w, double h,
                                          const std::string& fill, const std::string& stroke) {
    std::stringstream ss;
    ss << "  <rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w
       << "\" height=\"" << h << "\" fill=\"" << fill << "\" stroke=\"" << stroke
       << "\" stroke-width=\"1\" stroke-dasharray=\"4 3\"/>\n";
    return ss.str();
}

std::string ExportManager::SvgEllipse(double x, double y, double w, double h,
                                       const std::string& fill, const std::string& stroke) {
    std::stringstream ss;
    ss << "  <ellipse cx=\"" << (x + w / 2) << "\" cy=\"" << (y + h / 2)
       << "\" rx=\"" << (w / 2) << "\" ry=\"" << (h / 2)
       << "\" fill=\"" << fill << "\" stroke=\"" << stroke << "\" stroke-width=\"1\"/>\n";
    return ss.str();
}

std::string ExportManager::SvgArrow(double x1, double y1, double x2, double y2,
                                     const std::string& stroke) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.01) return "";
    dx /= len;
    dy /= len;
    double size = 8.0;
    double px = x2 - dx * size;
    double py = y2 - dy * size;
    double ox = -dy * (size * 0.5);
    double oy = dx * (size * 0.5);
    std::stringstream ss;
    ss << "  <polygon points=\""
       << x2 << "," << y2 << " "
       << (px + ox) << "," << (py + oy) << " "
       << (px - ox) << "," << (py - oy)
       << "\" fill=\"" << stroke << "\"/>\n";
    return ss.str();
}

std::string ExportManager::SvgText(double x, double y, const std::string& text,
                                    const std::string& font_family, int font_size) {
    // Escape special XML characters
    std::string escaped = text;
    size_t pos = 0;
    while ((pos = escaped.find('&', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&amp;");
        pos += 5;
    }
    pos = 0;
    while ((pos = escaped.find('<', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&lt;");
        pos += 4;
    }
    pos = 0;
    while ((pos = escaped.find('>', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&gt;");
        pos += 4;
    }
    
    std::stringstream ss;
    ss << "  <text x=\"" << x << "\" y=\"" << y << "\" font-family=\"" 
       << font_family << "\" font-size=\"" << font_size << "\">" << escaped << "</text>\n";
    return ss.str();
}

std::string ExportManager::SvgLine(double x1, double y1, double x2, double y2,
                                    const std::string& stroke, int width) {
    std::stringstream ss;
    ss << "  <line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 
       << "\" y2=\"" << y2 << "\" stroke=\"" << stroke << "\" stroke-width=\"" 
       << width << "\"/>\n";
    return ss.str();
}

} // namespace diagram
} // namespace scratchrobin
