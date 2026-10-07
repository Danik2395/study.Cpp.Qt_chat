#pragma once
#include <qobject.h>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QWidget>
#include <QEventLoop>
#include <QTimer>
#include <QDate>
#include <QTime>
#include <QTextCursor>
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
        login_ui->label_server_status->setProperty("server_status_property", login_ui->label_server_status->text());
        login_ui->label_server_status->setText("");

        login_ui->edit_room_id->setMaxLength(14);
        login_ui->edit_sender_id->setMaxLength(14);

        toggle_controls_pressable_state(false);

        connect(login_ui->btn_connect_server, &QPushButton::clicked, this, [this](){
                QEventLoop loop;

                login_ui->btn_connect_server->setEnabled(false);

                connect(&chat, &Chat_Client::message_recieved, &loop, &QEventLoop::quit);
                connect(&chat, &Chat_Client::connected_to_server, this, [this](){
                        toggle_controls_pressable_state(true);
                        });

                chat.send_discovery_request();

                QTimer::singleShot(TIME_TO_WAIT_SERVER_RESPONT_SEC , this, [this, &loop](){
                        if (chat.is_connected()) return;
                        QString server_status_text = login_ui->label_server_status->property("server_status_property").toString();
                        login_ui->label_server_status->setText(std::move(server_status_text.arg("Server connect request time expired.")));
                        login_ui->btn_connect_server->setEnabled(true);
                        });

                loop.exec();
                });

        connect(&chat, &Chat_Client::message_recieved, this, &App_Client::on_message_recieved);
        connect(login_ui->btn_connect_room, &QPushButton::clicked, this, &App_Client::request_room_join);

        connect(chat_ui->edit_new_message, &QLineEdit::returnPressed, this, &App_Client::send_message_to_room);
        connect(chat_ui->btn_send_message, &QPushButton::clicked, this, &App_Client::send_message_to_room);

        connect(chat_ui->btn_leave, &QPushButton::clicked, this, &App_Client::on_room_leave);
        connect(&chat, &Chat_Client::server_disconnected, this, &App_Client::on_bad_room_leave);
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
        QString text = QString("\n%1 %2\n%3: %4\n")
            .arg(QDate::currentDate().toString("dd.MM.yyyy"))
            .arg(QTime::currentTime().toString("HH:mm:ss"))
            .arg(msg.sender_id)
            .arg(QString::fromUtf8(msg.payload));

        chat_ui->edit_room_messages->moveCursor(QTextCursor::End);
        chat_ui->edit_room_messages->insertPlainText(text);
    }

    void show_info_message(Message msg)
    {
        QString text = QString("\n%1\n").arg(QString::fromUtf8(msg.payload));

        chat_ui->edit_room_messages->moveCursor(QTextCursor::End);
        chat_ui->edit_room_messages->insertPlainText(text);
    }

    void change_ui_on_room_joined(Message msg)
    {
        QString room_info_text = chat_ui->label_room_info->property("room_info_property").toString();
        chat_ui->label_room_info->setText(std::move(room_info_text.arg(msg.room_id).arg(msg.sender_id)));

        chat_ui->edit_room_messages->clear();
        chat_ui->edit_new_message->clear();

        stacked_widget->setCurrentWidget(chat_widget);
    }

    void change_ui_on_room_left()
    {
        stacked_widget->setCurrentWidget(login_widget);
    }

private slots:
    void request_room_join()
    {
        QEventLoop loop;

        connect(&chat, &Chat_Client::message_recieved, &loop, &QEventLoop::quit);

        toggle_controls_pressable_state(false);

        chat.send_join_room_request(login_ui->edit_room_id->text(),
                                    login_ui->edit_sender_id->text());

        QTimer::singleShot(TIME_TO_WAIT_ROOM_CONNECT_SEC , &loop, &QEventLoop::quit);

        loop.exec();

        toggle_controls_pressable_state(true);
    }

    void on_room_leave()
    {
        chat.send_leave_room_request();
        change_ui_on_room_left();
    }

    void on_bad_room_leave()
    {
        change_ui_on_room_left();
    }

    void send_message_to_room()
    {
        if (chat_ui->edit_new_message->text().isEmpty()) return;
        chat.send_message_to_room(chat_ui->edit_new_message->text());

        chat_ui->edit_new_message->clear();
    }

    void on_message_recieved(Message msg)
    {

        switch (msg.type)
        {
            case Message::Type::INFO:
                {
                QString server_status_text = login_ui->label_server_status->property("server_status_property").toString();
                login_ui->label_server_status->setText(std::move(server_status_text.arg(QString::fromUtf8(msg.payload))));
                break;
                }

            case Message::Type::CHAT_USR_MSG:
                show_message(std::move(msg));
                break;

            case Message::Type::CHAT_INFO_MSG:
                show_info_message(std::move(msg));
                break;

            case Message::Type::ROOM_JOINED:
                change_ui_on_room_joined(std::move(msg));
                break;

            case Message::Type::ROOM_LEFT:
                change_ui_on_room_left();
                break;

            default: return;
        }
    }
};
