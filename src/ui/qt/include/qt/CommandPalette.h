#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>

class CommandPalette : public QWidget
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget *parent = nullptr);
    
    void show();
    void hide();
    bool isVisible() const;

signals:
    void commandSelected(const QString &command);

private slots:
    void onTextChanged(const QString &text);
    void onItemActivated(QListWidgetItem *item);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QVBoxLayout* layout;
    QLineEdit* searchEdit;
    QListWidget* commandList;
    bool visible;
};

#endif // COMMANDPALETTE_H

