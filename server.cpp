#include "server.h"

Server::Server(QObject *parent) :
    QObject(parent)
{
    _server = new QTcpServer(this);
    connect(_server, SIGNAL(newConnection()), this, SLOT(newConnection()));

    if (_server->listen(QHostAddress::Any, 5566)) {
        qDebug() << "Server::Server pid(" << QCoreApplication::applicationPid() << ") listening for clients on port" << _server->serverAddress() << _server->serverPort();
    }
}

Server::~Server()
{
//    qDebug() << "Server::~Server()";
    if (_server) {
        delete _server;
    }
}

void Server::newConnection()
{
    // need to grab the socket
    QTcpSocket * socket = _server->nextPendingConnection();
    if (socket) {
        Session * session = new Session(socket);
        connect(session, SIGNAL(socketConnected()), this, SLOT(connected()));
        connect(session, SIGNAL(socketDisconnected()), this, SLOT(disconnected()));
        connect(session, SIGNAL(socketBytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
        connect(session, SIGNAL(socketReadyRead()), this, SLOT(readyRead()));
//        _sessions.push_back(session);
        qDebug() << "Server::newConnection() Client:" << socket->peerAddress() << socket->peerPort();
    }
}

void Server::connected()
{
//    qDebug() << "Server::connected()";
}

void Server::disconnected()
{
    Session * session = reinterpret_cast<Session *>(QObject::sender());
    if (session) {
        if (session->GetSocket()) {
            qDebug() << "Server::disconnected()" << session->GetSocket()->peerAddress() << session->GetSocket()->peerPort();
        }

        QString userId = session->GetId();
        QJsonObject disconnectdata;
        disconnectdata["userId"] = userId;

        //broadcast out user_disconnected event to all users listening to roomId
        BroadcastToRoom(session, "user_disconnected", disconnectdata);

        //perform final cleanup for this session
        _sessions.remove(userId); //remove connected userId
        for (QVector <QPointer <Session> > & sessions : _rooms) { //remove session from listening to all roomId's
            sessions.removeAll(session);
        }
        delete session;
    }
}

void Server::bytesWritten(qint64 )
{
//    qDebug() << "Server::bytesWritten()" << bytes;
}

void Server::readyRead()
{
//    qDebug() << "Server::readyRead()";
    Session * session = reinterpret_cast<Session *>(QObject::sender());
    if (session) {
        const QByteArray b = session->GetSocket()->readLine();

        // get the root object
        QJsonDocument doc = QJsonDocument::fromJson(b);
        QJsonObject o = doc.object();

//        qDebug() << " data:" << o;
        if (!o.contains("method") || !o.contains("data")) {
            session->SendClientError("method and data need to be defined");
        }
        else {
            const QString method = o["method"].toString();
            const QJsonObject data = o["data"].toObject();

            if (method == "logon") {
                DoLogon(session, data);
            }
            else if (method == "subscribe") {
                DoSubscribe(session, data);
            }
            else if (method == "unsubscribe") {
                DoUnsubscribe(session, data);
            }
            else if (method == "enter_room") {
                DoEnterRoom(session, data);
            }
            else if (method == "move") {
                DoMove(session, data);
            }
            else if (method == "chat") {
                //chat method is the only method where data is a string and not a JSON object
                DoChat(session, o["data"].toString());
            }
            else if (method == "portal") {
                DoPortal(session, data);
            }
            else if (method == "users_online") {
                DoUsersOnline(session, data);
            }
            else {
                session->SendClientError("Unrecognized method");
            }
        }
    }
}

void Server::DoLogon(Session * session, QJsonObject data)
{
//    qDebug() << "Server::DoLogon()";
    if (!data.contains("userId")) {
        session->SendClientError("No userId specified in logon method");
    }
    else if (!data.contains("roomId")) {
        session->SendClientError("No roomId specified in logon method");
    }
    else {
        QString userId = data["userId"].toString();
        QString roomId = data["roomId"].toString();

        session->SetId(userId);
        session->SetRoomId(roomId);

        if (QRegExp("[^a-zA-Z0-9_]").indexIn(userId) >= 0) {
            session->SendClientError("Illegal character in userId " + userId + ", only use alphanumeric and underscore");
        }
        else if (_sessions.contains(userId)) {
            session->SendClientError("User name is already in use");
        }
        else {
            qDebug() << "Server::DoLogon() User " << userId << "logged in";
            _sessions[userId] = session;
            session->SendOkay();
        }
    }

}

void Server::DoSubscribe(Session * session, QJsonObject data)
{
    //subscribe this session to events specified by roomId
//    qDebug() << "Server::DoSubscribe()";
    if (!data.contains("roomId")) {
        session->SendClientError("No roomId specified in subscribe method");
    }
    else {
        if (session) {
            QString roomId = data["roomId"].toString();
            _rooms[roomId].push_back(session);
            session->SendOkay();
        }
    }
}

void Server::DoUnsubscribe(Session *session, QJsonObject data)
{
//    qDebug() << "Server::DoUnsubscribe()";
    if (!data.contains("roomId")) {
        session->SendClientError("No roomId specified in unsubscribe method");
    }
    else {
        if (session) {
            QString roomId = data["roomId"].toString();
            _rooms[roomId].removeAll(session);
            session->SendOkay();
        }
    }
}

void Server::DoEnterRoom(Session *session, QJsonObject data)
{
//    qDebug() << "Server::DoEnterRoom()";
    if (!data.contains("roomId")) {
        session->SendClientError("No roomId specified in enter_room method");
    }
    else {
        QString oldRoomId = session->GetRoomId();
        QString userId = session->GetId();

        QJsonObject leave_data;
        leave_data["roomId"] = oldRoomId;
        leave_data["userId"] = userId;
        BroadcastToRoom(session, "user_leave", leave_data);

        QString roomId = data["roomId"].toString();
        QJsonObject enter_data;
        enter_data["roomId"] = roomId;
        enter_data["userId"] = userId;
        session->SetRoomId(roomId); //track internally (also need to subcribe to events?)

        //we need to broadcast user_enter method to all users listening to roomId
        BroadcastToRoom(session, "user_enter", enter_data);
    }


}

void Server::DoMove(Session * session, QJsonObject data)
{
    //broadcast this data to all sessions subscribed to events specified by roomId
//    qDebug() << "Server::DoMove()";

    QJsonObject movedata;
    movedata["roomId"] = session->GetRoomId();
    movedata["userId"] = session->GetId();
    movedata["position"] = data;

    //broadcast out user_moved event to all users listening to roomId
    BroadcastToRoom(session, "user_moved", movedata);
}

void Server::DoChat(Session *session, QString data)
{
//    qDebug() << "Server::DoChat()";

    QJsonObject chatdata;
    chatdata["roomId"] = session->GetRoomId();
    chatdata["userId"] = session->GetId();
    chatdata["message"] = data;

    //broadcast out user_chat event to all users listening to roomId
    BroadcastToRoom(session, "user_chat", chatdata);
}

void Server::DoPortal(Session *session, QJsonObject data)
{
//    qDebug() << "Server::DoPortal()";

    //broadcast out user_portal event to all users listening to roomId
    data["roomId"] = session->GetRoomId();
    data["userId"] = session->GetId();

    session->SendOkay();
    BroadcastToRoom(session, "user_portal", data);
}

void Server::DoUsersOnline(Session *session, QJsonObject data)
{
//    qDebug() << "Server::DoUsersOnline()";

    if (!data.contains("roomId")) { //users online across all rooms
        QStringList userIds = _sessions.keys();

        QJsonObject data;
        data["results"] = userIds.size();
        data["users"] = QJsonArray::fromStringList(userIds);

        session->SendData("users_online", data);
    }
    else { //users online in a specific room defined by roomId
        QStringList userIds;
        QString roomId = data["roomId"].toString();
        QVector <QPointer <Session> > & sessions = _rooms[roomId];

        for (QPointer <Session> & s : sessions) {
            if (s) {
                userIds.push_back(s->GetId());
            }
        }

        QJsonObject data;
        data["results"] = userIds.size();
        data["users"] = QJsonArray::fromStringList(userIds);
        data["roomId"] = roomId;

        session->SendData("users_online", data);
    }
}

void Server::BroadcastToRoom(Session *session, const QString method, const QJsonObject data)
{
//    qDebug() << "Server::BroadcastToRoom broadcasting" << method << "to" << session->GetRoomId();
    QJsonObject o;
    o["method"] = method;
    o["data"] = data;

    QJsonDocument doc(o);
    QByteArray b = doc.toJson(QJsonDocument::Compact) + "\n";

    QVector <QPointer <Session> > & sessions = _rooms[session->GetRoomId()];
    for (QPointer <Session> & s : sessions) {
        if (s && s->GetSocket() && s != session) {
//            qDebug() << " broadcasting" << method << "to user" << s->GetId();
            s->GetSocket()->write(b.data(), b.size());
        }
    }
}
