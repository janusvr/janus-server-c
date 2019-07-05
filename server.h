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
    Server(const int websocketport, const int udpport);
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
    void DoChat(Session * session, QString data);
    void DoPortal(Session * session, QJsonObject data);
    void DoUsersOnline(Session * session, QJsonObject data);

    void BroadcastToRoom(Session *session, const QString method, const QJsonObject data);

    void ProcessMessage(Session * session, const QByteArray & b);

    QMap <QString, QString> _ipPortToUserId; //note: for UDP packet -> userId lookup only
    QMap <QString, QPointer <Session> > _sessions; //"userId" index maps to specific Session
    QMap <QString, QVector <QPointer <Session> > > _rooms; //"roomId" index maps to Sessions listening

    QWebSocketServer * _server;
    QUdpSocket * _udpsocket;
    int _udpport;

};

#endif // SERVER_H
