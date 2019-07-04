#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QVector>
#include <QPointer>
#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUdpSocket>

class Session : public QObject
{
    Q_OBJECT

public:

    Session(QWebSocket * socket);
    ~Session();

    QWebSocket *GetSocket();
    void SendClientError(const QString error);
    void SendData(const QString method, const QJsonObject data = QJsonObject());

    void SetId(QString id);
    QString GetId();

    void SetRoomId(QString roomId);
    QString GetRoomId();

    QString GetIpPortCombo();

    void SetUdpPort(const int i);
    void SendMessage(const QByteArray b, const bool udpPreferred);

signals:

    void socketConnected();
    void socketDisconnected();
    void socketBytesWritten(qint64 bytes);
    void socketTextMessageReceived(QString message);

public slots:

    void connected();
    void disconnected();
    void bytesWritten(qint64 bytes);    
    void textMessageReceived(QString message);

private:

    QWebSocket * _socket;
    QString _id;
    QString _roomId;

    QUdpSocket * _udpSocket;
    quint16 _udpPort; //UDP supported if port > 0
};

#endif // SESSION_H
