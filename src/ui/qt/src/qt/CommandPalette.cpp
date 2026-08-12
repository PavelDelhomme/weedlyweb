#include "qt/CommandPalette.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QKeyEvent>
#include <QApplication>

CommandPalette::CommandPalette(QWidget *parent)
    : QWidget(parent)
    , layout(nullptr)
    , searchEdit(nullptr)
    , commandList(nullptr)
    , visible(false)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Tapez une commande... (CTRL+SHIFT+C pour fermer)");
    
    commandList = new QListWidget(this);
    commandList->addItem("Nouvel onglet");
    commandList->addItem("Fermer l'onglet");
    commandList->addItem("Paramètres");
    commandList->addItem("Aide");
    
    layout->addWidget(searchEdit);
    layout->addWidget(commandList);
    
    connect(searchEdit, &QLineEdit::textChanged, this, &CommandPalette::onTextChanged);
    connect(commandList, &QListWidget::itemActivated, this, &CommandPalette::onItemActivated);
    
    hide();
}

void CommandPalette::show()
{
    visible = true;
    QWidget::show();
    searchEdit->setFocus();
    resize(400, 300);
    move(parentWidget()->geometry().center() - QPoint(200, 150));
}

void CommandPalette::hide()
{
    visible = false;
    QWidget::hide();
}

bool CommandPalette::isVisible() const
{
    return visible;
}

void CommandPalette::onTextChanged(const QString &text)
{
    // Filtrer la liste des commandes
    for (int i = 0; i < commandList->count(); ++i) {
        QListWidgetItem* item = commandList->item(i);
        item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
    }
}

void CommandPalette::onItemActivated(QListWidgetItem *item)
{
    emit commandSelected(item->text());
    hide();
}

void CommandPalette::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape || 
        (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_C)) {
        hide();
    } else {
        QWidget::keyPressEvent(event);
    }
}

