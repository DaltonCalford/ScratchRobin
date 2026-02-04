/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SEQUENCE_EDITOR_DIALOG_H
#define SCRATCHROBIN_SEQUENCE_EDITOR_DIALOG_H

#include <cstdint>
#include <string>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class SequenceEditorMode {
    Create,
    Edit
};

class SequenceEditorDialog : public wxDialog {
public:
    SequenceEditorDialog(wxWindow* parent, SequenceEditorMode mode);

    std::string BuildSql() const;
    std::string sequence_name() const;

    void SetSequenceName(const std::string& name);
    void SetSchema(const std::string& schema);
    void SetDataType(const std::string& data_type);
    void SetStartValue(int64_t value);
    void SetIncrementBy(int64_t value);
    void SetMinValue(int64_t value);
    void SetMaxValue(int64_t value);
    void SetCacheSize(int value);
    void SetCycle(bool cycle);
    void SetIsOrdered(bool ordered);
    void SetCurrentValue(int64_t value);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string QuoteIdentifier(const std::string& value) const;
    bool IsSimpleIdentifier(const std::string& value) const;
    bool IsQuotedIdentifier(const std::string& value) const;

    SequenceEditorMode mode_;

    // Basic fields
    wxTextCtrl* name_ctrl_ = nullptr;
    wxTextCtrl* schema_ctrl_ = nullptr;
    wxChoice* data_type_choice_ = nullptr;

    // Numeric options
    wxSpinCtrl* start_value_ctrl_ = nullptr;
    wxSpinCtrl* increment_by_ctrl_ = nullptr;
    wxSpinCtrl* min_value_ctrl_ = nullptr;
    wxSpinCtrl* max_value_ctrl_ = nullptr;
    wxSpinCtrl* cache_size_ctrl_ = nullptr;

    // Checkboxes
    wxCheckBox* cycle_ctrl_ = nullptr;
    wxCheckBox* ordered_ctrl_ = nullptr;
    wxCheckBox* use_min_value_ctrl_ = nullptr;
    wxCheckBox* use_max_value_ctrl_ = nullptr;
    wxCheckBox* reset_sequence_ctrl_ = nullptr;

    // Edit mode fields
    wxStaticText* current_value_label_ = nullptr;
    wxStaticText* current_value_display_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SEQUENCE_EDITOR_DIALOG_H
