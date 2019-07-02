#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QVector>
#include <QPointer>
#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>

class Session : public QObject
{
    Q_OBJECT

public:

    Session(QTcpSocket * socket);
    QTcpSocket * GetSocket();
    void SendClientError(const QString error);
    void SendData(const QString method, const QJsonObject data);
    void SendOkay();

    void SetId(QString id);
    QString GetId();

    void SetRoomId(QString roomId);
    QString GetRoomId();

signals:

    void socketConnected();
    void socketDisconnected();
    void socketBytesWritten(qint64 bytes);
    void socketReadyRead();

public slots:

    void connected();
    void disconnected();
    void bytesWritten(qint64 bytes);
    void readyRead();

private:

    QTcpSocket * _socket;
    QString _id;
    QString _roomId;

};

#endif // SESSION_H
