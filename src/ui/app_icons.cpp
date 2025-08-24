#include "app_icons.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

namespace scratchrobin {

AppIcons& AppIcons::instance() {
    static AppIcons instance;
    return instance;
}

AppIcons::AppIcons() {
    // Initialize all icons
    appIcon_ = loadIcon(":/logos/Artwork/ScratchRobin.png", 256);
    windowIcon_ = loadIcon(":/logos/Artwork/ScratchRobin.png", 64);

    // Create fallback icons for actions
    connectIcon_ = createFallbackIcon("C", QColor("#4CAF50"), 32);
    disconnectIcon_ = createFallbackIcon("D", QColor("#F44336"), 32);
    executeIcon_ = createFallbackIcon("▶", QColor("#FF9800"), 32);
    stopIcon_ = createFallbackIcon("■", QColor("#F44336"), 32);
    saveIcon_ = createFallbackIcon("💾", QColor("#2196F3"), 32);
    openIcon_ = createFallbackIcon("📁", QColor("#FF9800"), 32);
    newIcon_ = createFallbackIcon("✨", QColor("#4CAF50"), 32);
    editIcon_ = createFallbackIcon("✏", QColor("#FF9800"), 32);
    deleteIcon_ = createFallbackIcon("🗑", QColor("#F44336"), 32);
    refreshIcon_ = createFallbackIcon("🔄", QColor("#2196F3"), 32);
    settingsIcon_ = createFallbackIcon("⚙", QColor("#9C27B0"), 32);
    aboutIcon_ = createFallbackIcon("ℹ", QColor("#607D8B"), 32);

    // Additional icons
    errorIcon_ = createFallbackIcon("❌", QColor("#F44336"), 32);
    warningIcon_ = createFallbackIcon("⚠", QColor("#FF9800"), 32);
    findIcon_ = createFallbackIcon("🔍", QColor("#2196F3"), 32);
    replaceIcon_ = createFallbackIcon("🔄", QColor("#4CAF50"), 32);

    // Database object icons
    databaseIcon_ = createFallbackIcon("🗄", QColor("#795548"), 32);
    tableIcon_ = createFallbackIcon("📋", QColor("#607D8B"), 32);
    viewIcon_ = createFallbackIcon("👁", QColor("#3F51B5"), 32);
    columnIcon_ = createFallbackIcon("📊", QColor("#FF5722"), 32);
    indexIcon_ = createFallbackIcon("🏷", QColor("#009688"), 32);
    constraintIcon_ = createFallbackIcon("🔒", QColor("#E91E63"), 32);
}

AppIcons::~AppIcons() {
}

QIcon AppIcons::loadIcon(const QString& path, int size) const {
    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        // Scale the pixmap to the desired size while maintaining aspect ratio
        QPixmap scaled = pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return QIcon(scaled);
    } else {
        // Return a fallback icon if loading fails
        return createFallbackIcon("SR", QColor("#2E7D32"), size);
    }
}

QIcon AppIcons::createFallbackIcon(const QString& text, const QColor& color, int size) const {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Draw background circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(0, 0, size, size);

    // Draw text
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(size * 0.4); // Text size relative to icon size
    painter.setFont(font);

    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(text);
    QPoint center((size - textRect.width()) / 2, (size - textRect.height()) / 2 + metrics.ascent());

    painter.drawText(center, text);

    return QIcon(pixmap);
}

QIcon AppIcons::getAppIcon() const {
    return appIcon_;
}

QIcon AppIcons::getWindowIcon() const {
    return windowIcon_;
}

QIcon AppIcons::getConnectIcon() const {
    return connectIcon_;
}

QIcon AppIcons::getDisconnectIcon() const {
    return disconnectIcon_;
}

QIcon AppIcons::getExecuteIcon() const {
    return executeIcon_;
}

QIcon AppIcons::getStopIcon() const {
    return stopIcon_;
}

QIcon AppIcons::getSaveIcon() const {
    return saveIcon_;
}

QIcon AppIcons::getOpenIcon() const {
    return openIcon_;
}

QIcon AppIcons::getNewIcon() const {
    return newIcon_;
}

QIcon AppIcons::getEditIcon() const {
    return editIcon_;
}

QIcon AppIcons::getDeleteIcon() const {
    return deleteIcon_;
}

QIcon AppIcons::getRefreshIcon() const {
    return refreshIcon_;
}

QIcon AppIcons::getSettingsIcon() const {
    return settingsIcon_;
}

QIcon AppIcons::getAboutIcon() const {
    return aboutIcon_;
}

QIcon AppIcons::getDatabaseIcon() const {
    return databaseIcon_;
}

QIcon AppIcons::getTableIcon() const {
    return tableIcon_;
}

QIcon AppIcons::getViewIcon() const {
    return viewIcon_;
}

QIcon AppIcons::getColumnIcon() const {
    return columnIcon_;
}

QIcon AppIcons::getIndexIcon() const {
    return indexIcon_;
}

QIcon AppIcons::getConstraintIcon() const {
    return constraintIcon_;
}

QIcon AppIcons::getErrorIcon() const {
    return errorIcon_;
}

QIcon AppIcons::getWarningIcon() const {
    return warningIcon_;
}

QIcon AppIcons::getFindIcon() const {
    return findIcon_;
}

QIcon AppIcons::getReplaceIcon() const {
    return replaceIcon_;
}

} // namespace scratchrobin
