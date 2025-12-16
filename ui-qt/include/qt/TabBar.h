#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>

class TabBar : public QWidget
{
    Q_OBJECT

public:
    explicit TabBar(QWidget *parent = nullptr);
    
    void addTab(const QString &title, int index);
    void removeTab(int index);
    void setActiveTab(int index);
    void setTabTitle(int index, const QString &title);

signals:
    void tabActivated(int index);
    void tabCloseRequested(int index);
    void newTabRequested();

private:
    QHBoxLayout* layout;
    QScrollArea* scrollArea;
    QWidget* tabsContainer;
    QPushButton* newTabButton;
    int activeTabIndex;
};

#endif // TABBAR_H

