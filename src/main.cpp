#include "gui/main/ExamBankSystem.h"
#include "env/ExamEnvManager.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    ExamBankSystem w;
    w.show();
#ifdef QT_DEBUG
    QObject::connect(&a, &QApplication::aboutToQuit, &ExamEnvManager::getInstance(), &ExamEnvManager::clearSettings);  // 注册退出信号
#endif

    return a.exec();
}
