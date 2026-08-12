#ifndef FAVORITESBAR_H
#define FAVORITESBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPair>
#include <QList>
#include <QString>

class FavoritesBar : public QWidget
{
    Q_OBJECT

public:
    explicit FavoritesBar(QWidget *parent = nullptr);

    void setFavorites(const QList<QPair<QString, QString>> &favorites); // name, url
    void refresh();

signals:
    void favoriteClicked(const QString &url);

private:
    QHBoxLayout* layout = nullptr;
    QList<QPushButton*> favoriteButtons;
};

#endif // FAVORITESBAR_H
