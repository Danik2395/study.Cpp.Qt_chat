#pragma once
#include <qobject.h>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QWidget>
#include "ui/ui_app_client_login.h"
#include "ui/ui_app_client_chat.h"


namespace Ui
{
    class App_Client_Login;
    class App_Client_Chat;
}

class App_Client : public QWidget
{
    Q_OBJECT

public:

    App_Client(QWidget* parent = nullptr) :
        QWidget(parent)
    {
        auto stacked_widget = new QStackedWidget(this);

        auto vbox_layout = new QVBoxLayout(this);
        vbox_layout->addWidget(stacked_widget);

        auto login_ui = Ui::App_Client_Login();
        auto chat_ui  = Ui::App_Client_Chat();

        chat_widget  = new QWidget(this);
        login_widget = new QWidget(this);

        login_ui.setupUi(login_widget);
        chat_ui.setupUi(chat_widget);

        stacked_widget->addWidget(login_widget);
        stacked_widget->addWidget(chat_widget);
    }

private:
    QWidget* chat_widget;
    QWidget* login_widget;
};
