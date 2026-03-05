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

#include <memory>
#include <string>
#include <vector>

#include <wx/panel.h>

#include "core/result_set.h"

namespace scratchrobin::ui {

/**
 * ChartType - Types of charts supported
 */
enum class ChartType {
  kBar,
  kLine,
  kPie,
  kArea,
  kScatter,
  kDoughnut,
};

/**
 * ChartDataSeries - Represents a data series for charting
 */
struct ChartDataSeries {
  std::string name;
  std::vector<double> values;
  wxColor color;
};

/**
 * ChartComponent - A component for displaying data visualizations
 */
class ChartComponent : public wxPanel {
 public:
  ChartComponent(wxWindow* parent, wxWindowID id = wxID_ANY);
  ~ChartComponent() override;

  // Set chart type
  void SetChartType(ChartType type);

  // Set chart title
  void SetTitle(const wxString& title);

  // Set axis labels
  void SetXAxisLabel(const wxString& label);
  void SetYAxisLabel(const wxString& label);

  // Set categories (X-axis labels)
  void SetCategories(const std::vector<wxString>& categories);

  // Add a data series
  void AddSeries(const ChartDataSeries& series);

  // Clear all series
  void ClearSeries();

  // Load data from a result set
  // column_x: column index for X values or categories
  // column_y: column index for Y values
  void LoadData(const std::shared_ptr<core::ResultSet>& result_set,
                int column_x,
                int column_y,
                const wxString& series_name = wxEmptyString);

  // Refresh/redraw the chart
  void RefreshChart();

  // Export chart as image
  bool SaveAsImage(const wxString& filename, int width = 800, int height = 600);

  // Enable/disable legend
  void EnableLegend(bool enable);

  // Set legend position (0=top, 1=right, 2=bottom, 3=left)
  void SetLegendPosition(int position);

 private:
  void OnPaint(wxPaintEvent& event);
  void OnSize(wxSizeEvent& event);

  void DrawBarChart(wxDC& dc, const wxRect& rect);
  void DrawLineChart(wxDC& dc, const wxRect& rect);
  void DrawPieChart(wxDC& dc, const wxRect& rect);
  void DrawAreaChart(wxDC& dc, const wxRect& rect);
  void DrawScatterChart(wxDC& dc, const wxRect& rect);
  void DrawDoughnutChart(wxDC& dc, const wxRect& rect);
  void DrawLegend(wxDC& dc, const wxRect& chart_rect);

  ChartType chart_type_{ChartType::kBar};
  wxString title_;
  wxString x_axis_label_;
  wxString y_axis_label_;
  std::vector<wxString> categories_;
  std::vector<ChartDataSeries> series_;

  bool legend_enabled_{true};
  int legend_position_{1};  // right side

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
