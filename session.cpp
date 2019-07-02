#include "session.h"

Session::Session(QTcpSocket * socket)
{    
//    qDebug() << "Session::Session";
    _socket = socket;
    connect(socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));
}

QTcpSocket * Session::GetSocket()
{
    return _socket;
}

void Session::SendClientError(const QString error)
{
    if (_socket) {
        QJsonObject o;
        o["method"] = QString("error");
        QJsonObject od;
        od["message"] = error;
        o["data"] = od;

        QJsonDocument doc(o);
        QByteArray b = doc.toJson(QJsonDocument::Compact) + "\n";

        qint64 bytesWritten = _socket->write(b.data(), b.size());
        qDebug() << "Session::SendClientError()" << error;
    }
}

void Session::SendData(const QString method, const QJsonObject data)
{
    if (_socket) {
        QJsonObject o;
        o["method"] = method;
        o["data"] = data;

        QJsonDocument doc(o);
        QByteArray b = doc.toJson(QJsonDocument::Compact) + "\n";

        _socket->write(b.data(), b.size());
//        qDebug() << "Session::SendData()" << b.size();
    }
}

void Session::SendOkay()
{
    if (_socket) {
        QJsonObject o;
        o["method"] = QString("okay");

        QJsonDocument doc(o);
        QByteArray b = doc.toJson(QJsonDocument::Compact) + "\n";

        _socket->write(b.data(), b.size());
//        qDebug() << "Session::SendOkay()" << b.size();
    }
}

void Session::connected()
{
//    qDebug() << "Session::connected()";
    emit socketConnected();
}

void Session::disconnected()
{
//    qDebug() << "Session::disconnected()";
    emit socketDisconnected();
}

void Session::bytesWritten(qint64 bytes)
{
//    qDebug() << "Session::bytesWritten()";
    emit socketBytesWritten(bytes);
}
void Session::readyRead()
{
//    qDebug() << "Session::readyRead()";
    emit socketReadyRead();
}

void Session::SetId(QString id)
{
    _id = id;
}

QString Session::GetId()
{
    return _id;
}

void Session::SetRoomId(QString roomId)
{
    _roomId = roomId;
}

QString Session::GetRoomId()
{
    return _roomId;
}
