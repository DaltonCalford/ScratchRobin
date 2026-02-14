/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PREPARED_PARAMS_DIALOG_H
#define SCRATCHROBIN_PREPARED_PARAMS_DIALOG_H

#include <vector>

#include <wx/dialog.h>

#include "core/prepared_types.h"

class wxGrid;
class wxButton;

namespace scratchrobin {

class PreparedParamsDialog : public wxDialog {
public:
    PreparedParamsDialog(wxWindow* parent,
                         size_t parameterCount,
                         const std::vector<PreparedParameter>& initial);

    const std::vector<PreparedParameter>& GetParams() const { return params_; }

private:
    void OnApply(wxCommandEvent& event);
    PreparedParamType ParseType(const wxString& value) const;
    PreparedParameter ParseRow(size_t row) const;
    bool TryParseRow(size_t row, PreparedParameter* outParam, std::string* error) const;
    wxString TypeToString(PreparedParamType type) const;

    wxGrid* grid_ = nullptr;
    wxButton* apply_button_ = nullptr;
    std::vector<PreparedParameter> params_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PREPARED_PARAMS_DIALOG_H
