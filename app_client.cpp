 #include <QApplication>
#include "app_client.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    App_Client w;
    w.show();
    return a.exec();
}

