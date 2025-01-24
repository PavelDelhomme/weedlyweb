#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDockWidget>

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow();

private:
    Ui::SettingsWindow *ui;
};

#endif // SETTINGSWINDOW_H
