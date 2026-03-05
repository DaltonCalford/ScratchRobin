/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/components/chart_component.h"

#include <algorithm>
#include <cmath>

#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/brush.h>
#include <wx/pen.h>
#include <wx/font.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(ChartComponent, wxPanel)
  EVT_PAINT(ChartComponent::OnPaint)
  EVT_SIZE(ChartComponent::OnSize)
wxEND_EVENT_TABLE()

ChartComponent::ChartComponent(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE) {
  SetBackgroundStyle(wxBG_STYLE_PAINT);
}

ChartComponent::~ChartComponent() {
  // Cleanup handled by wxWidgets
}

void ChartComponent::SetChartType(ChartType type) {
  chart_type_ = type;
  Refresh();
}

void ChartComponent::SetTitle(const wxString& title) {
  title_ = title;
  Refresh();
}

void ChartComponent::SetXAxisLabel(const wxString& label) {
  x_axis_label_ = label;
  Refresh();
}

void ChartComponent::SetYAxisLabel(const wxString& label) {
  y_axis_label_ = label;
  Refresh();
}

void ChartComponent::SetCategories(const std::vector<wxString>& categories) {
  categories_ = categories;
  Refresh();
}

void ChartComponent::AddSeries(const ChartDataSeries& series) {
  series_.push_back(series);
  Refresh();
}

void ChartComponent::ClearSeries() {
  series_.clear();
  Refresh();
}

void ChartComponent::LoadData(const std::shared_ptr<core::ResultSet>& result_set,
                              int column_x,
                              int column_y,
                              const wxString& series_name) {
  // TODO: Extract data from result set and populate series
  Refresh();
}

void ChartComponent::RefreshChart() {
  Refresh();
}

bool ChartComponent::SaveAsImage(const wxString& filename, int width, int height) {
  // TODO: Implement image export
  return false;
}

void ChartComponent::EnableLegend(bool enable) {
  legend_enabled_ = enable;
  Refresh();
}

void ChartComponent::SetLegendPosition(int position) {
  legend_position_ = position;
  Refresh();
}

void ChartComponent::OnPaint(wxPaintEvent& event) {
  wxAutoBufferedPaintDC dc(this);
  
  // Clear background
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
  
  wxRect client_rect = GetClientRect();
  
  // Draw title
  if (!title_.IsEmpty()) {
    dc.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, 
                      wxFONTWEIGHT_BOLD));
    dc.SetTextForeground(wxColor(0, 0, 0));
    dc.DrawLabel(title_, client_rect, wxALIGN_TOP | wxALIGN_CENTER_HORIZONTAL);
    client_rect.y += 25;
    client_rect.height -= 25;
  }
  
  // Draw chart based on type
  switch (chart_type_) {
    case ChartType::kBar:
      DrawBarChart(dc, client_rect);
      break;
    case ChartType::kLine:
      DrawLineChart(dc, client_rect);
      break;
    case ChartType::kPie:
      DrawPieChart(dc, client_rect);
      break;
    case ChartType::kArea:
      DrawAreaChart(dc, client_rect);
      break;
    case ChartType::kScatter:
      DrawScatterChart(dc, client_rect);
      break;
    case ChartType::kDoughnut:
      DrawDoughnutChart(dc, client_rect);
      break;
  }
  
  // Draw legend
  if (legend_enabled_) {
    DrawLegend(dc, client_rect);
  }
}

void ChartComponent::OnSize(wxSizeEvent& event) {
  Refresh();
  event.Skip();
}

void ChartComponent::DrawBarChart(wxDC& dc, const wxRect& rect) {
  if (series_.empty() || series_[0].values.empty()) {
    dc.DrawLabel(_("No data"), rect, wxALIGN_CENTER);
    return;
  }
  
  // TODO: Implement proper bar chart rendering
  dc.SetPen(wxPen(wxColor(100, 100, 100)));
  dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
  dc.DrawRectangle(rect);
  dc.DrawLabel(_("Bar Chart - Implementation Pending"), rect, wxALIGN_CENTER);
}

void ChartComponent::DrawLineChart(wxDC& dc, const wxRect& rect) {
  if (series_.empty() || series_[0].values.empty()) {
    dc.DrawLabel(_("No data"), rect, wxALIGN_CENTER);
    return;
  }
  
  // TODO: Implement proper line chart rendering
  dc.SetPen(wxPen(wxColor(100, 100, 100)));
  dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
  dc.DrawRectangle(rect);
  dc.DrawLabel(_("Line Chart - Implementation Pending"), rect, wxALIGN_CENTER);
}

void ChartComponent::DrawPieChart(wxDC& dc, const wxRect& rect) {
  if (series_.empty() || series_[0].values.empty()) {
    dc.DrawLabel(_("No data"), rect, wxALIGN_CENTER);
    return;
  }
  
  // TODO: Implement proper pie chart rendering
  dc.SetPen(wxPen(wxColor(100, 100, 100)));
  dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
  dc.DrawRectangle(rect);
  dc.DrawLabel(_("Pie Chart - Implementation Pending"), rect, wxALIGN_CENTER);
}

void ChartComponent::DrawAreaChart(wxDC& dc, const wxRect& rect) {
  // TODO: Implement area chart
  DrawLineChart(dc, rect);
}

void ChartComponent::DrawScatterChart(wxDC& dc, const wxRect& rect) {
  if (series_.empty() || series_[0].values.empty()) {
    dc.DrawLabel(_("No data"), rect, wxALIGN_CENTER);
    return;
  }
  
  // TODO: Implement proper scatter chart rendering
  dc.SetPen(wxPen(wxColor(100, 100, 100)));
  dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
  dc.DrawRectangle(rect);
  dc.DrawLabel(_("Scatter Chart - Implementation Pending"), rect, wxALIGN_CENTER);
}

void ChartComponent::DrawDoughnutChart(wxDC& dc, const wxRect& rect) {
  // TODO: Implement doughnut chart
  DrawPieChart(dc, rect);
}

void ChartComponent::DrawLegend(wxDC& dc, const wxRect& chart_rect) {
  if (series_.empty()) return;
  
  dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, 
                    wxFONTWEIGHT_NORMAL));
  dc.SetTextForeground(wxColor(0, 0, 0));
  
  int legend_x = chart_rect.x + 10;
  int legend_y = chart_rect.y + chart_rect.height - 20;
  
  for (const auto& series : series_) {
    wxColor color = series.color;
    if (color == wxNullColour) {
      color = wxColor(100, 100, 100);
    }
    
    dc.SetBrush(wxBrush(color));
    dc.SetPen(wxPen(color));
    dc.DrawRectangle(legend_x, legend_y, 12, 12);
    
    dc.DrawText(wxString::FromUTF8(series.name.c_str()), 
                legend_x + 16, legend_y);
    
    legend_x += 100;  // Simple spacing
  }
}

}  // namespace scratchrobin::ui
