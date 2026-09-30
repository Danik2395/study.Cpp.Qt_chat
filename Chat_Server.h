#pragma once
#include <cassert>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QMap>
#include <QSet>
#include <qhostaddress.h>
#include <qobject.h>
#include <stdexcept>
#include "Network_Node.h"

class Chat_Server : public QTcpServer, public Network_Node<Chat_Server>
{
    Q_OBJECT

        friend class Network_Node<Chat_Server>;

private:
    QMap<QString, QSet<QTcpSocket*>> rooms;
    QMap<QTcpSocket*, QString> client_rooms; // Reverse map to avoid cycle on delete

    QUdpSocket* udp_listener;

public:
    Chat_Server(QObject* parent = nullptr) : QTcpServer(parent)
    {
        connect(this, &QTcpServer::newConnection, this, &Chat_Server::on_new_connection);

        // Listen for client broadcast for accesable server
        udp_listener = new QUdpSocket(this);
        udp_listener->bind(QHostAddress::AnyIPv4, CHAT_PORT, QUdpSocket::ShareAddress);
        connect(udp_listener, &QUdpSocket::readyRead, this, &Chat_Server::handle_discovery_request);

        if (!this->listen(QHostAddress::AnyIPv4, CHAT_PORT))
        {
            throw std::runtime_error("Server start fail.");
        }
    }

    ~Chat_Server() { delete udp_listener; }

private slots:
    void on_new_connection()
    {
        QTcpSocket* client_socket = nextPendingConnection();
        connect(client_socket, &QTcpSocket::readyRead, this, [this, client_socket]() {
            read_message(client_socket);
        });
        connect(client_socket, &QTcpSocket::disconnected, this, [this, client_socket]() {
            leave_room(client_socket);
        });
    }

    void handle_discovery_request()
    {
        while (udp_listener->hasPendingDatagrams())
        {
            QNetworkDatagram request_datagram = udp_listener->receiveDatagram();
            QDataStream datagram_stream(request_datagram.data());
            datagram_stream.setVersion(QDataStream::Qt_6_0);

            Message request_msg;
            Message::read(datagram_stream, request_msg);

            if (request_msg.type != Message::Type::SERVER_SEARCH ||
                request_msg.sender_id != QString::number(SECRET_CODE)) continue;

            Message response_msg = {
                .type = Message::Type::SERVER_SEARCH_RESPONCE,
                .sender_id = QString::number(SECRET_CODE)
            };

            QByteArray response;
            QDataStream response_datastream(response);

            Message::write(response_datastream, response_msg);
            udp_listener->writeDatagram(response, request_datagram.senderAddress(), request_datagram.senderPort());
        }
    }

private:
    void read_message_callback(QTcpSocket* socket, const Message& msg)
    {
        switch (msg.type)
        {
            case Message::Type::JOIN_ROOM:
                join_room(socket, msg.room_id);
                send_chat_info_message(Message::Type::JOIN_ROOM, msg.room_id, msg.sender_id);
                break;

            case Message::Type::USR_MSG:
                broadcast_to_room(msg.room_id, msg);
                break;

            case Message::Type::LEAVE_ROOM:
                leave_room(socket);
                send_chat_info_message(Message::Type::LEAVE_ROOM, msg.room_id, msg.sender_id);
                break;

            default: assert(false && "read_message_callback: Invalid message type.");
        }
    }

    void join_room(QTcpSocket* socket, const QString& room_id)
    {
        rooms[room_id].insert(socket);
        client_rooms[socket] = room_id;
    }

    void leave_room(QTcpSocket* socket)
    {
        if (!client_rooms.contains(socket)) return;

        QString room_id = client_rooms.take(socket);
        rooms[room_id].remove(socket);

        if (rooms[room_id].isEmpty())
        {
            rooms.remove(room_id);
        }
        socket->deleteLater();
    }

    void broadcast_to_room(const QString& room_id, const Message& msg) const
    {
        if (!rooms.contains(room_id)) return;

        for (QTcpSocket* socket : rooms[room_id])
        {
            send_message(socket, msg);
        }
    }

    void send_chat_info_message(Message::Type msg_type, const QString& room_id, const QString& sender_id) const
    {
        Message chat_info_msg = {
            .type = Message::Type::CHAT_INFO_MSG,
            .sender_id = sender_id,
            .room_id = room_id,
        };

        switch (msg_type)
        {
            case Message::Type::JOIN_ROOM:
                chat_info_msg.payload = QString("[INFO] " + sender_id + " joined room " + room_id).toUtf8();
                break;

            case Message::Type::LEAVE_ROOM:
                chat_info_msg.payload = QString("[INFO] " + sender_id + " left room " + room_id).toUtf8();
                break;

            default: assert(false && "send_info: Invalid message type.");
        }

        broadcast_to_room(room_id, chat_info_msg);
    }
};
