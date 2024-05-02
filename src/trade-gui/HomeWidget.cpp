#include "HomeWidget.h"
#include "ui_home_widget.h"

HomeWidget::HomeWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
}

HomeWidget::~HomeWidget()
{
    delete ui;
}
