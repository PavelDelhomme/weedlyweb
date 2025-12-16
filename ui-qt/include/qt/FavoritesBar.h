#ifndef FAVORITESBAR_H
#define FAVORITESBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStringList>

class FavoritesBar : public QWidget
{
    Q_OBJECT

public:
    explicit FavoritesBar(QWidget *parent = nullptr);
    
    void setFavorites(const QStringList &favorites);
    void refresh();

signals:
    void favoriteClicked(const QString &url);

private:
    QHBoxLayout* layout;
    QList<QPushButton*> favoriteButtons;
};

#endif // FAVORITESBAR_H

