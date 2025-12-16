#include "qt/FavoritesBar.h"
#include <QHBoxLayout>
#include <QPushButton>

FavoritesBar::FavoritesBar(QWidget *parent)
    : QWidget(parent)
    , layout(nullptr)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(5);
}

void FavoritesBar::setFavorites(const QStringList &favorites)
{
    // Nettoyer les boutons existants
    for (QPushButton* button : favoriteButtons) {
        layout->removeWidget(button);
        button->deleteLater();
    }
    favoriteButtons.clear();
    
    // Créer de nouveaux boutons
    for (const QString &favorite : favorites) {
        QPushButton* button = new QPushButton(favorite, this);
        connect(button, &QPushButton::clicked, [this, favorite]() {
            emit favoriteClicked(favorite);
        });
        layout->addWidget(button);
        favoriteButtons.append(button);
    }
}

void FavoritesBar::refresh()
{
    // TODO: Recharger depuis le gestionnaire de favoris
}

