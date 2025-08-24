#pragma once

#include <QIcon>
#include <QPixmap>
#include <QApplication>

namespace scratchrobin {

class AppIcons {
public:
    static AppIcons& instance();

    // Application icon
    QIcon getAppIcon() const;

    // Window icon for dialogs
    QIcon getWindowIcon() const;

    // Toolbar icons
    QIcon getConnectIcon() const;
    QIcon getDisconnectIcon() const;
    QIcon getExecuteIcon() const;
    QIcon getStopIcon() const;
    QIcon getSaveIcon() const;
    QIcon getOpenIcon() const;
    QIcon getNewIcon() const;
    QIcon getEditIcon() const;
    QIcon getDeleteIcon() const;
    QIcon getRefreshIcon() const;
    QIcon getSettingsIcon() const;
    QIcon getAboutIcon() const;

    // Additional icons
    QIcon getErrorIcon() const;
    QIcon getWarningIcon() const;
    QIcon getFindIcon() const;
    QIcon getReplaceIcon() const;

    // Database object icons
    QIcon getDatabaseIcon() const;
    QIcon getTableIcon() const;
    QIcon getViewIcon() const;
    QIcon getColumnIcon() const;
    QIcon getIndexIcon() const;
    QIcon getConstraintIcon() const;

private:
    AppIcons();
    ~AppIcons();

    // Load icons from resources
    QIcon loadIcon(const QString& path, int size = 32) const;
    QIcon createFallbackIcon(const QString& text, const QColor& color, int size = 32) const;

    // Cached icons
    QIcon appIcon_;
    QIcon windowIcon_;
    QIcon connectIcon_;
    QIcon disconnectIcon_;
    QIcon executeIcon_;
    QIcon stopIcon_;
    QIcon saveIcon_;
    QIcon openIcon_;
    QIcon newIcon_;
    QIcon editIcon_;
    QIcon deleteIcon_;
    QIcon refreshIcon_;
    QIcon settingsIcon_;
    QIcon aboutIcon_;

    // Additional icons
    QIcon errorIcon_;
    QIcon warningIcon_;
    QIcon findIcon_;
    QIcon replaceIcon_;

    QIcon databaseIcon_;
    QIcon tableIcon_;
    QIcon viewIcon_;
    QIcon columnIcon_;
    QIcon indexIcon_;
    QIcon constraintIcon_;
};

} // namespace scratchrobin
