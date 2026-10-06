#pragma once
#include <qobject.h>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QWidget>
#include <QEventLoop>
#include <QTimer>
#include <QPushButton>
#include <qpushbutton.h>
#include <qstackedwidget.h>
#include <qwidget.h>
#include <utility>
#include "Network_Node.h"
#include "ui/ui_app_client_login.h"
#include "ui/ui_app_client_chat.h"
#include "Chat_Client.h"

constexpr int TIME_TO_WAIT_SERVER_RESPONT_SEC = 5000;
constexpr int TIME_TO_WAIT_ROOM_CONNECT_SEC = 5000;

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
        QWidget(parent),
        chat(this)
    {
        stacked_widget = new QStackedWidget(this);

        auto vbox_layout = new QVBoxLayout(this);
        vbox_layout->addWidget(stacked_widget);

        login_ui = new Ui::App_Client_Login();
        chat_ui  = new Ui::App_Client_Chat();

        chat_widget  = new QWidget(this);
        login_widget = new QWidget(this);

        login_ui->setupUi(login_widget);
        chat_ui->setupUi(chat_widget);

        stacked_widget->addWidget(login_widget);
        stacked_widget->addWidget(chat_widget);

        chat_ui->label_room_info->setProperty("room_info_property", chat_ui->label_room_info->text());

        toggle_controls_pressable_state(false);

        connect(login_ui->btn_connect_server, &QPushButton::clicked, this, [this](){
                QEventLoop loop;

                connect(&chat, &Chat_Client::message_receieved, &loop, &QEventLoop::quit);
                connect(&chat, &Chat_Client::connected_to_server, this, [this](){
                        toggle_controls_pressable_state(true);
                        login_ui->btn_connect_room->setEnabled(false);
                        });

                chat.send_discovery_request();

                QTimer::singleShot(TIME_TO_WAIT_SERVER_RESPONT_SEC , &loop, &QEventLoop::quit);

                loop.exec();
                });

        connect(&chat, &Chat_Client::message_receieved, this, &App_Client::on_message_recieved);
        connect(login_ui->btn_connect_room, &QPushButton::clicked, this, &App_Client::send_join_room_message);
    }

private:
    QStackedWidget* stacked_widget;
    Ui::App_Client_Chat*  chat_ui;
    Ui::App_Client_Login* login_ui;
    QWidget* chat_widget;
    QWidget* login_widget;

    Chat_Client chat;

    void toggle_controls_pressable_state(bool enable)
    {
        login_ui->btn_connect_room->setEnabled(enable);
        login_ui->edit_room_id->setEnabled(enable);
        login_ui->edit_sender_id->setEnabled(enable);
    }

    void show_message(Message msg)
    {

    }

    void show_info_message(Message msg)
    {

    }

    void change_ui_on_room_joined(Message msg)
    {
        stacked_widget->setCurrentWidget(chat_widget);
        QString room_info_text = chat_ui->label_room_info->property("room_info_property").toString();
        chat_ui->label_room_info->setText(std::move(room_info_text.arg(msg.room_id).arg(msg.sender_id)));

        chat_ui->edit_room_messages->clear();
        chat_ui->edit_new_message->clear();
    }

private slots:
    void send_join_room_message()
    {
        QEventLoop loop;

        connect(&chat, &Chat_Client::message_receieved, &loop, &QEventLoop::quit);

        toggle_controls_pressable_state(false);

        chat.send_join_room_request(login_ui->edit_room_id->text(),
                                    login_ui->edit_sender_id->text());

        QTimer::singleShot(TIME_TO_WAIT_ROOM_CONNECT_SEC , &loop, &QEventLoop::quit);

        loop.exec();

        toggle_controls_pressable_state(true);
    }

    void send_message_to_room()
    {

    }

    void on_message_recieved(Message msg)
    {

        switch (msg.type)
        {
            case Message::Type::INFO:
                // unhandled
                break;

            case Message::Type::CHAT_USR_MSG:
                show_message(std::move(msg));
                break;

            case Message::Type::CHAT_INFO_MSG:
                show_info_message(std::move(msg));
                break;

            case Message::Type::ROOM_JOINED:
                change_ui_on_room_joined(std::move(msg));
                break;

            default: return;
        }
    }
};
