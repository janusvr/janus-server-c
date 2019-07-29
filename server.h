#ifndef SERVER_H
#define SERVER_H

#include <QCoreApplication>
#include <QDate>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWebSocketServer>
#include <QUdpSocket>

#include "session.h"

class Server : public QObject
{
    Q_OBJECT

public:
    Server(const quint16 websocketport, const quint16 udpport);
    ~Server();

public slots:

    void newConnection();

    void connected();
    void disconnected();
    void bytesWritten(qint64 bytes);    
    void onTextMessageReceived(QString message);

    void readPendingDatagrams();

private:

    void DoLogon(Session *session, QJsonObject data);
    void DoSubscribe(Session * session, QJsonObject data);
    void DoUnsubscribe(Session * session, QJsonObject data);
    void DoEnterRoom(Session * session, QJsonObject data);
    void DoMove(Session * session, QJsonObject data);
    void DoChat(Session * session, QJsonObject data);
    void DoPortal(Session * session, QJsonObject data);
    void DoUsersOnline(Session * session, QJsonObject data);

    void BroadcastToUser(QString userId, const QString method, const QJsonObject data);
    void BroadcastToRoom(Session *session, const QString method, const QJsonObject data);

    void UpdateUDPPort(const QByteArray & b, quint16 senderPort);
    void ProcessMessage(Session *session, const QByteArray & b);

    QMap <QString, QPointer <Session> > _sessions; //"userId" index maps to specific Session
    QMap <QString, QVector <QPointer <Session> > > _rooms; //"roomId" index maps to Sessions listening

    QWebSocketServer * _server;
    QUdpSocket * _udpsocket;
    quint16 _udpport;

};

#endif // SERVER_H
