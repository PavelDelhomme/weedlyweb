#include "qt/BrowserWindow.h"
#include "qt/WebView.h"
#include "qt/TabBar.h"
#include "qt/NavigationBar.h"
#include "qt/FavoritesBar.h"
#include "qt/CommandPalette.h"
#include <QApplication>
#include <QScreen>
#include <QSize>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QVBoxLayout>
#include <iostream>

BrowserWindow::BrowserWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget(nullptr)
    , mainLayout(nullptr)
    , tabBar(nullptr)
    , navigationBar(nullptr)
    , favoritesBar(nullptr)
    , webContainer(nullptr)
    , commandPalette(nullptr)
    , activeTabIndex(-1)
    , homepage("https://www.google.fr")
{
    setupUI();
    setupConnections();
    loadConfiguration();
    
    // Ajouter le premier onglet
    if (tabs.empty()) {
        addNewTab(homepage);
    }
}

BrowserWindow::~BrowserWindow()
{
    saveConfiguration();
}

void BrowserWindow::setupUI()
{
    // Créer le widget central
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Layout principal vertical
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Créer les composants UI
    tabBar = new TabBar(this);
    navigationBar = new NavigationBar(this);
    favoritesBar = new FavoritesBar(this);
    webContainer = new QWidget(this);
    commandPalette = new CommandPalette(this);
    
    // Créer un layout pour le container web
    QVBoxLayout* webLayout = new QVBoxLayout(webContainer);
    webLayout->setContentsMargins(0, 0, 0, 0);
    
    // Ajouter les composants au layout principal
    mainLayout->addWidget(tabBar);
    mainLayout->addWidget(navigationBar);
    mainLayout->addWidget(favoritesBar);
    mainLayout->addWidget(webContainer, 1); // Prend tout l'espace restant
    
    // Configuration de la fenêtre
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QSize screenSize = screen->size();
        resize(screenSize.width() * 0.85, screenSize.height() * 0.85);
    } else {
        resize(1280, 720);
    }
    
    setWindowTitle("WeedlyWeb");
}

void BrowserWindow::setupConnections()
{
    // Connecter les signaux des composants
    connect(tabBar, &TabBar::tabActivated, this, &BrowserWindow::onTabActivated);
    connect(tabBar, &TabBar::tabCloseRequested, this, &BrowserWindow::onTabCloseRequested);
    connect(tabBar, &TabBar::newTabRequested, this, &BrowserWindow::onNewTabRequested);
    
    connect(navigationBar, &NavigationBar::navigateBack, this, &BrowserWindow::onNavigateBack);
    connect(navigationBar, &NavigationBar::navigateForward, this, &BrowserWindow::onNavigateForward);
    connect(navigationBar, &NavigationBar::refresh, this, &BrowserWindow::onRefresh);
    connect(navigationBar, &NavigationBar::goHome, this, &BrowserWindow::onGoHome);
    connect(navigationBar, &NavigationBar::urlActivated, [this](const QString &url) {
        if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
            if (tabs[activeTabIndex].webView) {
                tabs[activeTabIndex].webView->loadUrl(url);
            }
        }
    });
    
    connect(favoritesBar, &FavoritesBar::favoriteClicked, [this](const QString &url) {
        addNewTab(url);
    });
}

void BrowserWindow::loadConfiguration()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/WeedlyWeb/config.json";
    QFile file(configPath);
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        
        if (obj.contains("homepage")) {
            homepage = obj["homepage"].toString();
        }
        
        file.close();
    }
}

void BrowserWindow::saveConfiguration()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/WeedlyWeb";
    QDir().mkpath(configDir);
    
    QJsonObject obj;
    obj["homepage"] = homepage;
    
    QJsonDocument doc(obj);
    QFile file(configDir + "/config.json");
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void BrowserWindow::addNewTab(const QString &url)
{
    // Créer une nouvelle WebView
    WebView* webView = new WebView(webContainer);
    webView->setVisible(false); // Cachée par défaut
    
    // Créer les données de l'onglet
    TabData tabData;
    tabData.url = url.isEmpty() ? homepage : url;
    tabData.webView = webView;
    tabData.tabWidget = nullptr; // Sera géré par TabBar
    
    int newIndex = tabs.size();
    tabs.push_back(tabData);
    
    // Ajouter l'onglet à la barre d'onglets
    tabBar->addTab("Nouvel onglet", newIndex);
    
    // Ajouter la WebView au layout du container
    QVBoxLayout* webLayout = qobject_cast<QVBoxLayout*>(webContainer->layout());
    if (!webLayout) {
        webLayout = new QVBoxLayout(webContainer);
        webLayout->setContentsMargins(0, 0, 0, 0);
    }
    webLayout->addWidget(webView);
    
    // Activer ce nouvel onglet
    activateTab(newIndex);
    
    // Connecter les signaux
    connect(webView, &WebView::urlChanged, this, &BrowserWindow::onUrlChanged);
    connect(webView, &WebView::titleChanged, this, &BrowserWindow::onTitleChanged);
    connect(webView, &WebView::loadStarted, this, &BrowserWindow::onLoadStarted);
    connect(webView, &WebView::loadFinished, this, &BrowserWindow::onLoadFinished);
    
    // Charger l'URL
    webView->loadUrl(tabData.url);
}

void BrowserWindow::removeTab(int index)
{
    if (index < 0 || index >= static_cast<int>(tabs.size())) {
        return;
    }
    
    // Supprimer la WebView
    if (tabs[index].webView) {
        tabs[index].webView->deleteLater();
    }
    
    // Retirer de la barre d'onglets
    tabBar->removeTab(index);
    
    tabs.erase(tabs.begin() + index);
    
    // Si c'était l'onglet actif, activer un autre
    if (activeTabIndex == index) {
        if (tabs.empty()) {
            close();
        } else {
            activeTabIndex = tabs.size() - 1;
            activateTab(activeTabIndex);
        }
    } else if (activeTabIndex > index) {
        activeTabIndex--;
    }
}

void BrowserWindow::activateTab(int index)
{
    if (index < 0 || index >= static_cast<int>(tabs.size())) {
        return;
    }
    
    activeTabIndex = index;
    tabBar->setActiveTab(index);
    
    // Cacher toutes les WebViews
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (tabs[i].webView) {
            tabs[i].webView->setVisible(i == static_cast<size_t>(index));
        }
    }
    
    // Afficher la WebView active
    if (tabs[index].webView) {
        tabs[index].webView->setVisible(true);
        onUrlChanged(tabs[index].webView->getCurrentUrl());
    }
}

void BrowserWindow::onUrlChanged(const QString &url)
{
    // Mettre à jour la barre d'URL
    // TODO: navigationBar->setUrl(url);
}

void BrowserWindow::onTitleChanged(const QString &title)
{
    setWindowTitle(title + " - WeedlyWeb");
    
    // Mettre à jour le titre de l'onglet
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        QString shortTitle = title.length() > 20 ? title.left(17) + "..." : title;
        tabBar->setTabTitle(activeTabIndex, shortTitle);
    }
}

void BrowserWindow::onLoadStarted()
{
    // Afficher l'indicateur de chargement
    navigationBar->setLoading(true);
}

void BrowserWindow::onLoadFinished(bool success)
{
    // Masquer l'indicateur de chargement
    navigationBar->setLoading(false);
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
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        if (tabs[activeTabIndex].webView) {
            tabs[activeTabIndex].webView->back();
        }
    }
}

void BrowserWindow::onNavigateForward()
{
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        if (tabs[activeTabIndex].webView) {
            tabs[activeTabIndex].webView->forward();
        }
    }
}

void BrowserWindow::onRefresh()
{
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        if (tabs[activeTabIndex].webView) {
            tabs[activeTabIndex].webView->reload();
        }
    }
}

void BrowserWindow::onGoHome()
{
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        if (tabs[activeTabIndex].webView) {
            tabs[activeTabIndex].webView->loadUrl(homepage);
        }
    }
}

void BrowserWindow::onUrlBarActivated()
{
    // Charger l'URL depuis la barre d'URL
    QString url = navigationBar->getUrl();
    if (activeTabIndex >= 0 && activeTabIndex < static_cast<int>(tabs.size())) {
        if (tabs[activeTabIndex].webView) {
            tabs[activeTabIndex].webView->loadUrl(url);
        }
    }
}

void BrowserWindow::updateStarButton()
{
    // TODO: Mettre à jour le bouton favori
}

void BrowserWindow::refreshFavoritesBar()
{
    // TODO: Rafraîchir la barre de favoris
}

