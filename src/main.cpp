#include "gui/main/ExamBankSystem.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    ExamBankSystem w;
    w.show();
    return a.exec();
}
