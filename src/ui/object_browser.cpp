#include "object_browser.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QLabel>
#include <QHeaderView>

namespace scratchrobin {

class ObjectBrowser::Impl : public QWidget {
public:
    Impl() {
        setupUI();
    }

    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);

        // Info label
        infoLabel_ = new QLabel("Database Objects");
        infoLabel_->setAlignment(Qt::AlignCenter);
        layout->addWidget(infoLabel_);

        // Object tree
        objectTree_ = new QTreeWidget();
        objectTree_->setHeaderLabel("Database Objects");
        objectTree_->setAlternatingRowColors(true);

        // Add some sample database objects
        QTreeWidgetItem* databasesItem = new QTreeWidgetItem(objectTree_);
        databasesItem->setText(0, "Databases");

        QTreeWidgetItem* schemasItem = new QTreeWidgetItem(databasesItem);
        schemasItem->setText(0, "Schemas");

        QTreeWidgetItem* tablesItem = new QTreeWidgetItem(schemasItem);
        tablesItem->setText(0, "Tables");

        QTreeWidgetItem* viewsItem = new QTreeWidgetItem(schemasItem);
        viewsItem->setText(0, "Views");

        databasesItem->setExpanded(true);
        schemasItem->setExpanded(true);

        layout->addWidget(objectTree_);
    }

    void refresh() {
        // TODO: Refresh object tree from database
    }

private:
    QLabel* infoLabel_;
    QTreeWidget* objectTree_;
};

ObjectBrowser::ObjectBrowser()
    : impl_(std::make_unique<Impl>()) {
}

ObjectBrowser::~ObjectBrowser() = default;

void ObjectBrowser::refresh() {
    impl_->refresh();
}

void ObjectBrowser::expandNode(const std::string& path) {
    // TODO: Implement node expansion
}

void ObjectBrowser::selectObject(const std::string& objectPath) {
    // TODO: Implement object selection
}

QWidget* ObjectBrowser::getWidget() {
    return impl_.get();
}

} // namespace scratchrobin