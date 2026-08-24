#include "qt/BrowserWindow.h"
#include "qt/WebView.h"
#include "qt/TabBar.h"
#include "qt/NavigationBar.h"
#include "qt/FavoritesBar.h"
#include "qt/CommandPalette.h"

#include "managers/FileManager.h"
#include "core/FavoritesJson.h"
#include "core/PageEnhancements.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QScreen>
#include <QDir>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <iostream>
#include <utility>
#include <vector>
#include <algorithm>
BrowserWindow::BrowserWindow(QWidget *parent)
    : QMainWindow(parent)
    , homepage(QStringLiteral("https://www.duckduckgo.com"))
{
    setupUI();
    setupConnections();
    loadConfiguration();
    loadFavorites();
    restoreSession();

    if (tabs.empty()) {
        addNewTab(homepage);
    }
}

BrowserWindow::~BrowserWindow()
{
    persistSession();
    saveConfiguration();
}

void BrowserWindow::closeEvent(QCloseEvent *event)
{
    persistSession();
    saveConfiguration();
    QMainWindow::closeEvent(event);
}

void BrowserWindow::setupUI()
{
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);

    mainLayout = new QVBoxLayout(centralWidget_);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    tabBar = new TabBar(this);
    navigationBar = new NavigationBar(this);
    favoritesBar = new FavoritesBar(this);
    webContainer = new QWidget(this);
    commandPalette = new CommandPalette(this);

    auto* webLayout = new QVBoxLayout(webContainer);
    webLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(tabBar);
    mainLayout->addWidget(navigationBar);
    mainLayout->addWidget(favoritesBar);
    mainLayout->addWidget(webContainer, 1);

    // Configuration de la fenêtre : moniteur sous le curseur, puis maximisée
    QScreen* targetScreen = QApplication::screenAt(QCursor::pos());
    if (!targetScreen) {
        targetScreen = QApplication::primaryScreen();
    }
    if (targetScreen) {
        // Positionner d'abord sur le moniteur de la souris, sinon maximize
        // peut s'appliquer au mauvais écran / bureau virtuel
        setGeometry(targetScreen->availableGeometry());
        setWindowState(windowState() | Qt::WindowMaximized);
    } else {
        resize(1280, 720);
    }

    setWindowTitle(QStringLiteral("WeedlyWeb (Qt)"));
}

void BrowserWindow::setupConnections()
{
    connect(tabBar, &TabBar::tabActivated, this, &BrowserWindow::onTabActivated);
    connect(tabBar, &TabBar::tabCloseRequested, this, &BrowserWindow::onTabCloseRequested);
    connect(tabBar, &TabBar::newTabRequested, this, &BrowserWindow::onNewTabRequested);

    connect(navigationBar, &NavigationBar::navigateBack, this, &BrowserWindow::onNavigateBack);
    connect(navigationBar, &NavigationBar::navigateForward, this, &BrowserWindow::onNavigateForward);
    connect(navigationBar, &NavigationBar::refresh, this, &BrowserWindow::onRefresh);
    connect(navigationBar, &NavigationBar::goHome, this, &BrowserWindow::onGoHome);
    connect(navigationBar, &NavigationBar::urlActivated, this, [this](const QString &url) {
        if (WebView* view = currentWebView()) {
            view->loadUrl(url);
        }
    });

    connect(favoritesBar, &FavoritesBar::favoriteClicked, this, [this](const QString &url) {
        addNewTab(url);
    });

    connect(navigationBar, &NavigationBar::toggleDarkMode, this, [this](bool enabled) {
        forceDarkMode = enabled;
        navigationBar->setDarkModeActive(forceDarkMode);
        applyPageEnhancements();
        saveConfiguration();
    });
    connect(navigationBar, &NavigationBar::toggleReaderMode, this, [this](bool enabled) {
        readerMode = enabled;
        navigationBar->setReaderModeActive(readerMode);
        if (readerMode) {
            applyPageEnhancements();
        } else if (WebView* view = currentWebView()) {
            view->reload();
        }
    });
}

void BrowserWindow::loadConfiguration()
{
    try {
        const auto cfg = FileManager::readJSON(FileManager::configJSONPath());
        if (cfg.contains("homepage") && cfg["homepage"].is_string()) {
            homepage = QString::fromStdString(cfg["homepage"].get<std::string>());
        }
        forceDarkMode = cfg.value("force_dark_mode", false);
    } catch (...) {
        std::cerr << "[Qt] config.json illisible, homepage par défaut.\n";
    }
    if (navigationBar) {
        navigationBar->setDarkModeActive(forceDarkMode);
    }
}

void BrowserWindow::saveConfiguration()
{
    nlohmann::json cfg = FileManager::readJSON(FileManager::configJSONPath());
    if (!cfg.is_object()) {
        cfg = nlohmann::json::object();
    }
    cfg["homepage"] = homepage.toStdString();
    cfg["force_dark_mode"] = forceDarkMode;
    FileManager::writeJSON(FileManager::configJSONPath(), cfg);
}

void BrowserWindow::loadFavorites()
{
    favoritesRoot = FileManager::readJSON(FileManager::favoritesJSONPath());
    if (!favoritesRoot.is_array()) {
        if (favoritesRoot.is_object() && favoritesRoot.contains("favorites") &&
            favoritesRoot["favorites"].is_array()) {
            favoritesRoot = favoritesRoot["favorites"];
        } else {
            favoritesRoot = nlohmann::json::array();
        }
    }
    refreshFavoritesBar();
}

void BrowserWindow::restoreSession()
{
    const std::string sessionPath = FileManager::obtenirCheminAbsolu("assets/datas/session.json");
    const auto session = FileManager::readJSON(sessionPath);
    if (!session.is_object() || !session.contains("tabs") || !session["tabs"].is_array()) {
        return;
    }
    for (const auto& tab : session["tabs"]) {
        if (tab.is_string()) {
            addNewTab(QString::fromStdString(tab.get<std::string>()));
        } else if (tab.is_object() && tab.contains("url") && tab["url"].is_string()) {
            addNewTab(QString::fromStdString(tab["url"].get<std::string>()));
        }
    }
    if (session.contains("active") && session["active"].is_number_integer()) {
        activateTab(session["active"].get<int>());
    }
}

void BrowserWindow::persistSession()
{
    nlohmann::json session = nlohmann::json::object();
    session["tabs"] = nlohmann::json::array();
    for (const auto& tab : tabs) {
        QString url = tab.url;
        if (tab.webView) {
            url = tab.webView->getCurrentUrl();
        }
        session["tabs"].push_back(url.toStdString());
    }
    session["active"] = activeTabIndex;
    FileManager::writeJSON(FileManager::obtenirCheminAbsolu("assets/datas/session.json"), session);
}

void BrowserWindow::addNewTab(const QString &url)
{
    auto* webView = new WebView(webContainer);
    webView->setVisible(false);

    TabData tabData;
    tabData.url = url.isEmpty() ? homepage : url;
    tabData.webView = webView;

    const int newIndex = static_cast<int>(tabs.size());
    tabs.push_back(tabData);

    tabBar->addTab(QStringLiteral("Nouvel onglet"), newIndex);

    if (auto* webLayout = qobject_cast<QVBoxLayout*>(webContainer->layout())) {
        webLayout->addWidget(webView);
    }

    connect(webView, &WebView::urlChanged, this, &BrowserWindow::onUrlChanged);
    connect(webView, &WebView::titleChanged, this, &BrowserWindow::onTitleChanged);
    connect(webView, &WebView::loadStarted, this, &BrowserWindow::onLoadStarted);
    connect(webView, &WebView::loadFinished, this, &BrowserWindow::onLoadFinished);

    activateTab(newIndex);
    webView->loadUrl(tabData.url);
}

void BrowserWindow::removeTab(int index)
{
    if (index < 0 || index >= static_cast<int>(tabs.size())) {
        return;
    }

    if (tabs[index].webView) {
        tabs[index].webView->deleteLater();
    }

    tabBar->removeTab(index);
    tabs.erase(tabs.begin() + index);

    if (tabs.empty()) {
        addNewTab(homepage);
        return;
    }

    if (activeTabIndex == index) {
        activateTab(std::min(index, static_cast<int>(tabs.size()) - 1));
    } else if (activeTabIndex > index) {
        --activeTabIndex;
    }
}

void BrowserWindow::activateTab(int index)
{
    if (index < 0 || index >= static_cast<int>(tabs.size())) {
        return;
    }

    activeTabIndex = index;
    tabBar->setActiveTab(index);

    for (size_t i = 0; i < tabs.size(); ++i) {
        if (tabs[i].webView) {
            tabs[i].webView->setVisible(i == static_cast<size_t>(index));
        }
    }

    if (tabs[index].webView) {
        onUrlChanged(tabs[index].webView->getCurrentUrl());
        updateStarButton();
    }
}

WebView* BrowserWindow::currentWebView() const
{
    if (activeTabIndex < 0 || activeTabIndex >= static_cast<int>(tabs.size())) {
        return nullptr;
    }
    return tabs[activeTabIndex].webView;
}

void BrowserWindow::onUrlChanged(const QString &url)
{
    navigationBar->setUrl(url);
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        tabs[activeTabIndex].url = url;
    }
    updateStarButton();
}

void BrowserWindow::onTitleChanged(const QString &title)
{
    setWindowTitle(title + QStringLiteral(" - WeedlyWeb"));
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        const QString shortTitle = title.length() > 24 ? title.left(21) + QStringLiteral("...") : title;
        tabBar->setTabTitle(activeTabIndex, shortTitle);
    }
}

void BrowserWindow::onLoadStarted()
{
    navigationBar->setLoading(true);
}

void BrowserWindow::onLoadFinished(bool /*success*/)
{
    navigationBar->setLoading(false);
    updateStarButton();
    applyPageEnhancements();
}

void BrowserWindow::onNewTabRequested()
{
    addNewTab();
}

void BrowserWindow::onTabCloseRequested(int index)
{
    removeTab(index);
}

void BrowserWindow::onTabActivated(int index)
{
    activateTab(index);
}

void BrowserWindow::onNavigateBack()
{
    if (WebView* view = currentWebView()) {
        view->back();
    }
}

void BrowserWindow::onNavigateForward()
{
    if (WebView* view = currentWebView()) {
        view->forward();
    }
}

void BrowserWindow::onRefresh()
{
    if (WebView* view = currentWebView()) {
        view->reload();
    }
}

void BrowserWindow::onGoHome()
{
    if (WebView* view = currentWebView()) {
        view->loadUrl(homepage);
    }
}

void BrowserWindow::onUrlBarActivated()
{
    if (WebView* view = currentWebView()) {
        view->loadUrl(navigationBar->getUrl());
    }
}

void BrowserWindow::updateStarButton()
{
    // L’étoile détaillée reste côté GTK ; ici on indique juste l’état favori.
    Q_UNUSED(favoritesRoot);
}

void BrowserWindow::refreshFavoritesBar()
{
    std::vector<std::pair<std::string, std::string>> entries;
    FavoritesJson::collectBookmarkEntries(favoritesRoot, entries, 24);

    QList<QPair<QString, QString>> qtEntries;
    for (const auto& e : entries) {
        qtEntries.append({QString::fromStdString(e.first), QString::fromStdString(e.second)});
    }
    favoritesBar->setFavorites(qtEntries);
}

void BrowserWindow::applyPageEnhancements()
{
    WebView* view = currentWebView();
    if (!view || !view->page()) {
        return;
    }
    const QString url = view->getCurrentUrl();
    if (url.startsWith(QStringLiteral("about:")) || url.startsWith(QStringLiteral("weedly:")) ||
        url.startsWith(QStringLiteral("data:"))) {
        return;
    }
    if (readerMode) {
        view->page()->runJavaScript(QString::fromUtf8(PageEnhancements::readerModeEnableScript()));
    }
    if (forceDarkMode) {
        view->page()->runJavaScript(QString::fromUtf8(PageEnhancements::darkModeEnableScript()));
    } else {
        view->page()->runJavaScript(QString::fromUtf8(PageEnhancements::darkModeDisableScript()));
    }
}
