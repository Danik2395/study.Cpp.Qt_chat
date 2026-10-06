#pragma once
#include <concepts>
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <qtypes.h>
#include <qudpsocket.h>

constexpr quint16 CHAT_PORT     = 45454;
constexpr quint16 SECRET_CODE   = 12356;
constexpr int     MAX_ROOM_ID   = 15;
constexpr int     MAX_SENDER_ID = 15;

// #pragma pack(push, 1) // Stash current allignment in stack and set allignment to one
struct Message
{
    enum Type
    {
        JOIN_ROOM,
        ROOM_JOINED,
        LEAVE_ROOM,
        CHAT_USR_MSG,
        CHAT_INFO_MSG,
        INFO,
        SERVER_SEARCH_REQUEST,
        SERVER_SEARCH_RESPONCE
    };

    Message::Type type;
    QString sender_id;
    QString room_id;
    QByteArray payload;

    static void write(QDataStream& out, const Message& msg)
    {
        out << msg.type << msg.sender_id << msg.room_id << msg.payload;
    }
    static void read(QDataStream& in, Message& msg)
    {
        in >> msg.type >> msg.sender_id >> msg.room_id >> msg.payload;
    }
};
// #pragma pack(pop) // Pop top allignment from stack

template <typename Derived>
concept Network_Node_Derived = requires (Derived derived, QTcpSocket* sock, const Message& msg)
{
    { derived.read_message_callback(sock, msg) } -> std::same_as<void>;
};

template <typename Derived>
class Network_Node
{
protected:
    virtual ~Network_Node()
    {
        static_assert(Network_Node_Derived<Derived>);
    }

    void send_message_to_socket(QTcpSocket* socket, const Message& msg) const
    {
        if (!socket || !socket->isOpen()) return;

        QByteArray out_bytes;
        QDataStream bytes_out_stream(&out_bytes, QIODevice::WriteOnly);
        bytes_out_stream.setVersion(QDataStream::Qt_6_0);
        Message::write(bytes_out_stream, msg);

        socket->write(out_bytes);
    }

    void read_message(QTcpSocket* socket) // Not const callback
    {
        QDataStream socket_in_stream(socket);
        socket_in_stream.setVersion(QDataStream::Qt_6_0);

        while (true)
        {
            socket_in_stream.startTransaction();
            Message msg;
            Message::read(socket_in_stream, msg);

            // If not all packets arrived stop reading (return to point from where read started)
            // operator >> checks whether bytes in stream are enough to read into the msg
            if (socket_in_stream.commitTransaction())
            {
                static_cast<Derived*>(this)->read_message_callback(socket, msg);
            }
            else
            {
                break;
            }
        }
    }
};
