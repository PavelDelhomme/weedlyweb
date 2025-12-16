#include "qt/NavigationBar.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

NavigationBar::NavigationBar(QWidget *parent)
    : QWidget(parent)
    , layout(nullptr)
    , backButton(nullptr)
    , forwardButton(nullptr)
    , refreshButton(nullptr)
    , homeButton(nullptr)
    , urlBar(nullptr)
    , loadingSpinner(nullptr)
    , starButton(nullptr)
    , menuButton(nullptr)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    
    backButton = new QPushButton("←", this);
    forwardButton = new QPushButton("→", this);
    refreshButton = new QPushButton("↻", this);
    homeButton = new QPushButton("⌂", this);
    
    urlBar = new QLineEdit(this);
    urlBar->setPlaceholderText("Entrez une URL...");
    
    loadingSpinner = new QLabel("", this);
    loadingSpinner->setFixedSize(16, 16);
    loadingSpinner->setText("⟳");
    loadingSpinner->hide();
    
    starButton = new QPushButton("☆", this);
    menuButton = new QPushButton("☰", this);
    
    layout->addWidget(backButton);
    layout->addWidget(forwardButton);
    layout->addWidget(refreshButton);
    layout->addWidget(homeButton);
    layout->addWidget(urlBar, 1);
    layout->addWidget(loadingSpinner);
    layout->addWidget(starButton);
    layout->addWidget(menuButton);
    
    connect(backButton, &QPushButton::clicked, this, &NavigationBar::navigateBack);
    connect(forwardButton, &QPushButton::clicked, this, &NavigationBar::navigateForward);
    connect(refreshButton, &QPushButton::clicked, this, &NavigationBar::refresh);
    connect(homeButton, &QPushButton::clicked, this, &NavigationBar::goHome);
    connect(urlBar, &QLineEdit::returnPressed, [this]() {
        emit urlActivated(urlBar->text());
    });
}

void NavigationBar::setUrl(const QString &url)
{
    urlBar->setText(url);
}

QString NavigationBar::getUrl() const
{
    return urlBar->text();
}

void NavigationBar::setLoading(bool loading)
{
    if (loading) {
        loadingSpinner->setText("⟳");
        loadingSpinner->show();
    } else {
        loadingSpinner->hide();
    }
}

