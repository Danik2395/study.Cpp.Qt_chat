#pragma once
#include <cassert>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <qhostaddress.h>
#include <qobject.h>
#include <qstringview.h>
#include <qudpsocket.h>
#include "Network_Node.h"

class Chat_Client : public QObject, public Network_Node<Chat_Client>
{
    Q_OBJECT

    friend class Network_Node<Chat_Client>;

private:
    QTcpSocket* socket_tcp;
    QUdpSocket* socket_udp;

public:
    Chat_Client(QObject* parent = nullptr) : QObject(parent)
    {
        socket_tcp = new QTcpSocket(this);
        connect(socket_tcp, &QTcpSocket::readyRead, this, [this]() {
            read_message(socket_tcp);
        });

        socket_udp = new QUdpSocket(this);
        connect(socket_udp, &QUdpSocket::readyRead, this, &Chat_Client::handle_discovery_responce);
    }

    void send_discovery_request() const
    {
        Message discovery_msg = {
            .type = Message::Type::SERVER_SEARCH_REQUEST,
            .sender_id = QString::number(SECRET_CODE)
        };

        QByteArray discovery_request;
        QDataStream discovery_request_datastream(discovery_request);

        Message::write(discovery_request_datastream, discovery_msg);

        socket_udp->writeDatagram(discovery_request, QHostAddress::Broadcast, CHAT_PORT);
    }

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

            socket_tcp->connectToHost(responce_datagram.senderAddress().toString(), CHAT_PORT);
            emit connected_to_server();
        }
    }

signals:
    void message_receieved(const Message& msg);

private:
    void read_message_callback(QTcpSocket* s, const Message& msg)
    {
        emit message_receieved(msg);
        // Slots called immediately
        // And code after emmit will be execuded only after all slots have returned
    }
};
