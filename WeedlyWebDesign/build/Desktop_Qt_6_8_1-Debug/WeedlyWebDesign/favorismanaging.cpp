#include "favorismanaging.h"
#include "ui_favorismanaging.h"

favorisManaging::favorisManaging(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::favorisManaging)
{
    ui->setupUi(this);
}

favorisManaging::~favorisManaging()
{
    delete ui;
}
