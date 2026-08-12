#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

class WebView;
class TabBar;
class NavigationBar;
class FavoritesBar;
class CommandPalette;

struct TabData {
    QString url;
    WebView* webView = nullptr;
};

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onUrlChanged(const QString &url);
    void onTitleChanged(const QString &title);
    void onLoadStarted();
    void onLoadFinished(bool success);
    void onNewTabRequested();
    void onTabCloseRequested(int index);
    void onTabActivated(int index);
    void onNavigateBack();
    void onNavigateForward();
    void onRefresh();
    void onGoHome();
    void onUrlBarActivated();

private:
    void setupUI();
    void setupConnections();
    void loadConfiguration();
    void saveConfiguration();
    void loadFavorites();
    void restoreSession();
    void persistSession();
    void addNewTab(const QString &url = QString());
    void removeTab(int index);
    void activateTab(int index);
    void updateStarButton();
    void refreshFavoritesBar();
    void applyPageEnhancements();
    WebView* currentWebView() const;

    QWidget* centralWidget_ = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    TabBar* tabBar = nullptr;
    NavigationBar* navigationBar = nullptr;
    FavoritesBar* favoritesBar = nullptr;
    QWidget* webContainer = nullptr;
    CommandPalette* commandPalette = nullptr;

    std::vector<TabData> tabs;
    int activeTabIndex = -1;
    QString homepage;
    nlohmann::json favoritesRoot = nlohmann::json::array();
    bool forceDarkMode = false;
    bool readerMode = false;
};

#endif // BROWSERWINDOW_H
