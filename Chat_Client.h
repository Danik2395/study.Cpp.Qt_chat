#pragma once
#include <cassert>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <qhostaddress.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtcpsocket.h>
#include <qudpsocket.h>
#include <utility>
#include "Network_Node.h"

class Chat_Client : public QObject, public Network_Node<Chat_Client>
{
    Q_OBJECT

    friend class Network_Node<Chat_Client>;

private:
    QTcpSocket* socket_tcp;
    QUdpSocket* socket_udp;

    QString user_sender_id;
    QString user_room_id;

public:
    Chat_Client(QObject* parent = nullptr) : QObject(parent)
    {
        socket_tcp = new QTcpSocket(this);
        connect(socket_tcp, &QTcpSocket::readyRead, this, [this]() {
            read_message(socket_tcp);
        });

        socket_udp = new QUdpSocket(this);
        connect(socket_udp, &QUdpSocket::readyRead, this, &Chat_Client::handle_discovery_responce);

        connect(socket_tcp, &QTcpSocket::connected, this, [this]() {
            emit connected_to_server();
        });

        connect(socket_tcp, &QTcpSocket::disconnected, this, [this]() {
            user_sender_id.clear();
            user_room_id.clear();
            emit server_disconnected();
        });
    }

    void send_discovery_request() const
    {
        Message discovery_msg = {
            .type = Message::Type::SERVER_SEARCH_REQUEST,
            .sender_id = QString::number(SECRET_CODE)
        };

        QByteArray discovery_request;
        QDataStream discovery_request_datastream(&discovery_request, QIODevice::WriteOnly);

        Message::write(discovery_request_datastream, discovery_msg);

        // To test on loopback
        socket_udp->writeDatagram(discovery_request, QHostAddress::LocalHost, CHAT_PORT);

        socket_udp->writeDatagram(discovery_request, QHostAddress::Broadcast, CHAT_PORT);
    }

    void send_message_to_room(QString message_text) const
    {
        Message msg = {
            .type = Message::Type::CHAT_USR_MSG,
            .sender_id = user_sender_id,
            .room_id = user_room_id,
            .payload = message_text.toUtf8()
        };

        return send_message_to_socket(socket_tcp, msg);
    }

    void send_join_room_request(QString room_id, QString sender_id)
    {
        send_message_to_socket(socket_tcp, {
                .type = Message::Type::JOIN_ROOM,
                .sender_id = std::move(sender_id),
                .room_id = std::move(room_id)
                });
    }

    void send_leave_room_request()
    {
        send_message_to_socket(socket_tcp, {
                .type = Message::Type::LEAVE_ROOM,
                .sender_id = user_sender_id,
                .room_id = user_room_id
                });
    }


signals:
    void message_recieved(const Message& msg);
    void connected_to_server() const;
    void server_disconnected();

private slots:
    void handle_discovery_responce() const
    {
        while (socket_udp->hasPendingDatagrams())
        {
            QNetworkDatagram responce_datagram = socket_udp->receiveDatagram();
            QDataStream datagram_stream(responce_datagram.data());
            datagram_stream.setVersion(QDataStream::Qt_6_0);

            Message responce_msg;
            Message::read(datagram_stream, responce_msg);

            if (responce_msg.type != Message::Type::SERVER_SEARCH_RESPONCE ||
                responce_msg.sender_id != QString::number(SECRET_CODE)) continue;

            // On request if already connected
            if (socket_tcp->state() != QAbstractSocket::UnconnectedState) continue;

            socket_tcp->connectToHost(responce_datagram.senderAddress().toString(), CHAT_PORT);
        }
    }

private:
    void read_message_callback(QTcpSocket* s, const Message& msg)
    {

        switch (msg.type)
        {
            case Message::Type::ROOM_JOINED:
                user_sender_id = msg.sender_id;
                user_room_id = msg.room_id;
                break;

            case Message::Type::ROOM_LEFT:
                user_sender_id.clear();
                user_room_id.clear();
                break;

            default: break;
        }

        emit message_recieved(msg);
        // Slots called immediately
        // And code after emmit will be execuded only after all slots have returned
    }
};
