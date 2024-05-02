#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{

class HomeWidget;

}
QT_END_NAMESPACE

class HomeWidget final: public QWidget
{
    Q_OBJECT

public:
    explicit HomeWidget(QWidget* parent = nullptr);
    ~HomeWidget() override;

private:
    Ui::HomeWidget* ui;
};
