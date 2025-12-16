#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QString>
#include <memory>
#include <vector>

class WebView;
class TabBar;
class NavigationBar;
class FavoritesBar;
class CommandPalette;

struct TabData {
    QString url;
    WebView* webView;
    QWidget* tabWidget;
};

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow();

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
    void addNewTab(const QString &url = QString());
    void removeTab(int index);
    void activateTab(int index);
    void updateStarButton();
    void refreshFavoritesBar();
    
    // UI Components
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    TabBar* tabBar;
    NavigationBar* navigationBar;
    FavoritesBar* favoritesBar;
    QWidget* webContainer;
    CommandPalette* commandPalette;
    
    // Data
    std::vector<TabData> tabs;
    int activeTabIndex;
    QString homepage;
    
    // Managers (à adapter depuis GTK)
    // std::unique_ptr<FavoritesManager> favoritesManager;
    // std::unique_ptr<TabsManager> tabsManager;
};

#endif // BROWSERWINDOW_H

