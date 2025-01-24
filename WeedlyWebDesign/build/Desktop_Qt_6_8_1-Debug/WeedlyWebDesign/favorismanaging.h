#ifndef FAVORISMANAGING_H
#define FAVORISMANAGING_H

#include <QDialog>

namespace Ui {
class favorisManaging;
}

class favorisManaging : public QDialog
{
    Q_OBJECT

public:
    explicit favorisManaging(QWidget *parent = nullptr);
    ~favorisManaging();

private:
    Ui::favorisManaging *ui;
};

#endif // FAVORISMANAGING_H
