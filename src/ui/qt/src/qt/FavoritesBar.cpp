#include "qt/FavoritesBar.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <algorithm>

FavoritesBar::FavoritesBar(QWidget *parent)
    : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 2, 6, 2);
    layout->setSpacing(4);
    layout->addStretch();
}

void FavoritesBar::setFavorites(const QList<QPair<QString, QString>> &favorites)
{
    for (QPushButton* button : favoriteButtons) {
        layout->removeWidget(button);
        button->deleteLater();
    }
    favoriteButtons.clear();

    // Insert before trailing stretch
    int insertAt = std::max(0, layout->count() - 1);
    for (const auto &favorite : favorites) {
        const QString& name = favorite.first;
        const QString& url = favorite.second;
        auto* button = new QPushButton(name, this);
        button->setFlat(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(url);
        connect(button, &QPushButton::clicked, this, [this, url]() {
            emit favoriteClicked(url);
        });
        layout->insertWidget(insertAt++, button);
        favoriteButtons.append(button);
    }
}

void FavoritesBar::refresh()
{
}
