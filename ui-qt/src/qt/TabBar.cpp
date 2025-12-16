#include "qt/TabBar.h"
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

TabBar::TabBar(QWidget *parent)
    : QWidget(parent)
    , layout(nullptr)
    , scrollArea(nullptr)
    , tabsContainer(nullptr)
    , newTabButton(nullptr)
    , activeTabIndex(-1)
{
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    tabsContainer = new QWidget();
    layout = new QHBoxLayout(tabsContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    
    scrollArea->setWidget(tabsContainer);
    
    newTabButton = new QPushButton("+", this);
    newTabButton->setFixedSize(30, 30);
    connect(newTabButton, &QPushButton::clicked, this, &TabBar::newTabRequested);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(newTabButton);
}

void TabBar::addTab(const QString &title, int index)
{
    QPushButton* tabButton = new QPushButton(title, tabsContainer);
    tabButton->setCheckable(true);
    tabButton->setMinimumHeight(30);
    
    QPushButton* closeButton = new QPushButton("×", tabsContainer);
    closeButton->setFixedSize(20, 20);
    closeButton->setFlat(true);
    
    QHBoxLayout* tabLayout = new QHBoxLayout();
    tabLayout->addWidget(tabButton);
    tabLayout->addWidget(closeButton);
    tabLayout->setContentsMargins(5, 2, 5, 2);
    
    QWidget* tabWidget = new QWidget(tabsContainer);
    tabWidget->setLayout(tabLayout);
    
    layout->insertWidget(index, tabWidget);
    
    connect(tabButton, &QPushButton::clicked, [this, index]() {
        emit tabActivated(index);
    });
    
    connect(closeButton, &QPushButton::clicked, [this, index]() {
        emit tabCloseRequested(index);
    });
}

void TabBar::removeTab(int index)
{
    QLayoutItem* item = layout->itemAt(index);
    if (item) {
        QWidget* widget = item->widget();
        if (widget) {
            layout->removeWidget(widget);
            widget->deleteLater();
        }
    }
}

void TabBar::setActiveTab(int index)
{
    activeTabIndex = index;
    // Mettre à jour l'apparence des onglets
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem* item = layout->itemAt(i);
        if (item) {
            QWidget* tabWidget = item->widget();
            if (tabWidget) {
                QPushButton* tabButton = tabWidget->findChild<QPushButton*>();
                if (tabButton) {
                    tabButton->setChecked(i == index);
                }
            }
        }
    }
}

void TabBar::setTabTitle(int index, const QString &title)
{
    QLayoutItem* item = layout->itemAt(index);
    if (item) {
        QWidget* tabWidget = item->widget();
        if (tabWidget) {
            QPushButton* tabButton = tabWidget->findChild<QPushButton*>();
            if (tabButton) {
                tabButton->setText(title);
            }
        }
    }
}

