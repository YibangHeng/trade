#include <QApplication>

#include "HomeWidget.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    HomeWidget widget;
    widget.show();

    return QApplication::exec();
}
