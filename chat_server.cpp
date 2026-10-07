#include <QCoreApplication>
#include "Chat_Server.h"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    Chat_Server server;

    return a.exec();
}
