#ifndef NAVIGATIONBAR_H
#define NAVIGATIONBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QToolButton>

class NavigationBar : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationBar(QWidget *parent = nullptr);

    void setUrl(const QString &url);
    QString getUrl() const;
    void setLoading(bool loading);
    void setDarkModeActive(bool active);
    void setReaderModeActive(bool active);

signals:
    void navigateBack();
    void navigateForward();
    void refresh();
    void goHome();
    void urlActivated(const QString &url);
    void toggleDarkMode(bool enabled);
    void toggleReaderMode(bool enabled);
    void starClicked();

private:
    QHBoxLayout* layout = nullptr;
    QPushButton* backButton = nullptr;
    QPushButton* forwardButton = nullptr;
    QPushButton* refreshButton = nullptr;
    QPushButton* homeButton = nullptr;
    QLineEdit* urlBar = nullptr;
    QLabel* loadingSpinner = nullptr;
    QPushButton* starButton = nullptr;
    QToolButton* darkModeButton = nullptr;
    QToolButton* readerModeButton = nullptr;
    QPushButton* menuButton = nullptr;
};

#endif // NAVIGATIONBAR_H
