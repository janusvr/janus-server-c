#include "session.h"

Session::Session(QWebSocket * socket) :
    _udpPort(0)
{    
//    qDebug() << "Session::Session";
    _socket = socket;
    connect(socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));    
    connect(socket, SIGNAL(textMessageReceived(QString)),this, SLOT(textMessageReceived(QString)));

    _udpSocket = new QUdpSocket();
}

Session::~Session()
{
    delete _udpSocket;
}

QWebSocket * Session::GetSocket()
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

        SendMessage(b, false);
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

        SendMessage(b, false);
//        qDebug() << "Session::SendData()" << b.size();
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

void Session::textMessageReceived(QString message)
{
    emit socketTextMessageReceived(message);
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

QString Session::GetIpPortCombo()
{
    return _socket->peerAddress().toString() + ":" + QString::number(_udpPort);
}

void Session::SetUdpPort(const int i)
{
    _udpPort = i;
}

void Session::SendMessage(const QByteArray b, const bool udpPreferred)
{
    if (udpPreferred && _udpPort > 0 && b.size() <= 4096) {
//        qDebug() << "Session::SendMessage sending UDP bytes to" << _socket->peerAddress() << _udpPort << ":" << _udpSocket->writeDatagram(b, _socket->peerAddress(), _udpPort);
        _udpSocket->writeDatagram(b, _socket->peerAddress(), _udpPort);
    }
    else {
        _socket->sendTextMessage(b);
    }
}
