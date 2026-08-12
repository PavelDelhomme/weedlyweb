#include "qt/NavigationBar.h"

NavigationBar::NavigationBar(QWidget *parent)
    : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    backButton = new QPushButton(QStringLiteral("←"), this);
    forwardButton = new QPushButton(QStringLiteral("→"), this);
    refreshButton = new QPushButton(QStringLiteral("↻"), this);
    homeButton = new QPushButton(QStringLiteral("⌂"), this);

    urlBar = new QLineEdit(this);
    urlBar->setPlaceholderText(QStringLiteral("Entrez une URL..."));

    loadingSpinner = new QLabel(this);
    loadingSpinner->setFixedSize(16, 16);
    loadingSpinner->setText(QStringLiteral("⟳"));
    loadingSpinner->hide();

    starButton = new QPushButton(QStringLiteral("☆"), this);
    starButton->setToolTip(QStringLiteral("Favoris"));

    darkModeButton = new QToolButton(this);
    darkModeButton->setText(QStringLiteral("◐"));
    darkModeButton->setCheckable(true);
    darkModeButton->setToolTip(QStringLiteral("Mode sombre des pages (comme Dark Reader)"));

    readerModeButton = new QToolButton(this);
    readerModeButton->setText(QStringLiteral("≡"));
    readerModeButton->setCheckable(true);
    readerModeButton->setToolTip(QStringLiteral("Mode lecture (texte épuré)"));

    menuButton = new QPushButton(QStringLiteral("☰"), this);

    layout->addWidget(backButton);
    layout->addWidget(forwardButton);
    layout->addWidget(refreshButton);
    layout->addWidget(homeButton);
    layout->addWidget(urlBar, 1);
    layout->addWidget(loadingSpinner);
    layout->addWidget(starButton);
    layout->addWidget(darkModeButton);
    layout->addWidget(readerModeButton);
    layout->addWidget(menuButton);

    connect(backButton, &QPushButton::clicked, this, &NavigationBar::navigateBack);
    connect(forwardButton, &QPushButton::clicked, this, &NavigationBar::navigateForward);
    connect(refreshButton, &QPushButton::clicked, this, &NavigationBar::refresh);
    connect(homeButton, &QPushButton::clicked, this, &NavigationBar::goHome);
    connect(urlBar, &QLineEdit::returnPressed, this, [this]() {
        emit urlActivated(urlBar->text());
    });
    connect(starButton, &QPushButton::clicked, this, &NavigationBar::starClicked);
    connect(darkModeButton, &QToolButton::toggled, this, &NavigationBar::toggleDarkMode);
    connect(readerModeButton, &QToolButton::toggled, this, &NavigationBar::toggleReaderMode);
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
        loadingSpinner->setText(QStringLiteral("⟳"));
        loadingSpinner->show();
    } else {
        loadingSpinner->hide();
    }
}

void NavigationBar::setDarkModeActive(bool active)
{
    darkModeButton->blockSignals(true);
    darkModeButton->setChecked(active);
    darkModeButton->blockSignals(false);
}

void NavigationBar::setReaderModeActive(bool active)
{
    readerModeButton->blockSignals(true);
    readerModeButton->setChecked(active);
    readerModeButton->blockSignals(false);
}
