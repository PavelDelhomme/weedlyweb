#ifndef NAVIGATIONBAR_H
#define NAVIGATIONBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QMovie>

class NavigationBar : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationBar(QWidget *parent = nullptr);
    
    void setUrl(const QString &url);
    QString getUrl() const;
    void setLoading(bool loading);

signals:
    void navigateBack();
    void navigateForward();
    void refresh();
    void goHome();
    void urlActivated(const QString &url);

private:
    QHBoxLayout* layout;
    QPushButton* backButton;
    QPushButton* forwardButton;
    QPushButton* refreshButton;
    QPushButton* homeButton;
    QLineEdit* urlBar;
    QLabel* loadingSpinner;
    QPushButton* starButton;
    QPushButton* menuButton;
};

#endif // NAVIGATIONBAR_H

