#include "server.h"

Server::Server(const quint16 websocketport, const quint16 udpport)
{    

    _server = new QWebSocketServer("janus-server-c", QWebSocketServer::NonSecureMode, this);
    connect(_server, SIGNAL(newConnection()), this, SLOT(newConnection()));

    _udpsocket = new QUdpSocket();
    connect(_udpsocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

    qDebug() << "JanusVR Presence Server (C++) v1.3 - process id" << QCoreApplication::applicationPid();
    qDebug() << "Usage: ";
    qDebug() << "\tjanus-server-c [-wsport x] [-udpport x]";

    if (_server->listen(QHostAddress::Any, websocketport)) {
        qDebug() << "Server::Server listening for WebSocket clients on" << _server->serverAddress() << _server->serverPort();
    }

    _udpport = udpport;
    if (_udpsocket->bind(QHostAddress::Any, _udpport)) {
        qDebug() << "Server::Server listening for UDP clients on" << _server->serverAddress() << udpport;

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
    qDebug() << "Server::newConnection()";
    // need to grab the socket
    QWebSocket * socket = _server->nextPendingConnection();
    if (socket) {
        Session * session = new Session(socket);
        connect(session, SIGNAL(socketConnected()), this, SLOT(connected()));
        connect(session, SIGNAL(socketDisconnected()), this, SLOT(disconnected()));
        connect(session, SIGNAL(socketBytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));        
        connect(session, SIGNAL(socketTextMessageReceived(QString)), this, SLOT(onTextMessageReceived(QString)));
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

        //perform cleanup for this session
        QString userId = session->GetId();
        QJsonObject disconnectdata;
        disconnectdata["userId"] = userId;

        //broadcast out user_disconnected event to all users listening to roomId
        BroadcastToRoom(session, "user_disconnected", disconnectdata);

        _sessions.remove(userId); //remove connected userId
        for (QVector <QPointer <Session> > & sessions : _rooms) { //remove session from listening to all roomId's
            sessions.removeAll(session);
        }
        delete session;
//            qDebug() << "Server::disconnected()" << _ipPortToUserId << _sessions;        
    }
}

void Server::bytesWritten(qint64 )
{
//    qDebug() << "Server::bytesWritten()" << bytes;
}

void Server::onTextMessageReceived(QString message)
{
    Session * session = reinterpret_cast<Session *>(QObject::sender());
    if (session) {
        const QByteArray b = message.toLatin1();
        ProcessMessage(session, b);
    }
}

void Server::UpdateUDPPort(const QByteArray & b, quint16 senderPort)
{
    QJsonDocument doc = QJsonDocument::fromJson(b);
    QJsonObject o = doc.object();

    if (!o.contains("method") || !o.contains("data")) {
        qDebug("  Server::UpdateUDPPort - method or data need to be defined");
        return;
    }

    const QJsonObject data = o["data"].toObject();
    if (!data.contains("userId")) {
//        qDebug() << " Server::UpdateUDPPort - userID needs to be defined within data";
        return;
    }

    const QString userId = data["userId"].toString();
    if (!_sessions.contains(userId) || _sessions[userId].isNull()) {
//        qDebug() << " Server::UpdateUDPPort - no session for given userId" << userId;
        return;
    }

    if (_sessions[userId]->GetUdpPort() == 0) {
        qDebug() << "Server::UpdateUDPPort - updating" << userId << "UDP port to " << senderPort;
        _sessions[userId]->SetUdpPort(senderPort);
    }
}

void Server::ProcessMessage(Session * session, const QByteArray & b)
{
    // get the root object
    QJsonDocument doc = QJsonDocument::fromJson(b);
    QJsonObject o = doc.object();

//        qDebug() << " data:" << o;
    if (!o.contains("method") || !o.contains("data")) {
        qDebug("  Server::ProcessMessage - method or data need to be defined");
        qDebug() << "  Packet:" << b.left(256);
        return;
    }

    const QString method = o["method"].toString();
    const QJsonObject data = o["data"].toObject();
    if (!data.contains("userId")) {
//        qDebug() << " Server::ProcessMessage - userID needs to be defined within data";
        return;
    }

    const QString userId = data["userId"].toString();

    if (method == "logon" && session) {
        //note: we do logon first, because we need to add it to sessions list
        DoLogon(session, data);
        return;
    }

    if (!_sessions.contains(userId)) {
//        qDebug() << " Server::ProcessMessage - no active session with the supplied userId " << userId;
        return;
    }
    else if (_sessions[userId].isNull()) {
//        qDebug() << " Server::ProcessMessage - session null for given userId " << userId;
        return;
    }

    QPointer <Session> s = _sessions[userId];
    if (method == "subscribe") {
        DoSubscribe(s, data);
    }
    else if (method == "unsubscribe") {
        DoUnsubscribe(s, data);
    }
    else if (method == "enter_room") {
        DoEnterRoom(s, data);
    }
    else if (method == "move") {
        DoMove(s, data);
    }
    else if (method == "chat") {
        DoChat(s, data);
    }
    else if (method == "portal") {
        DoPortal(s, data);
    }
    else if (method == "users_online") {
        DoUsersOnline(s, data);
    }
    else {
        s->SendClientError("Unrecognized method");
    }
}

void Server::readPendingDatagrams()
{
//    qDebug() << "Server::readPendingDatagrams()";
    while (_udpsocket->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress sender;
        quint16 senderPort;

        const int maxSize = int(_udpsocket->pendingDatagramSize());
        datagram.resize(maxSize);
        _udpsocket->readDatagram(datagram.data(), maxSize, &sender, &senderPort);

        //v1.3 Update outbound UDP port to use for this session
        UpdateUDPPort(datagram, senderPort);
        ProcessMessage(nullptr, datagram);
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

        if (QRegExp("[^a-zA-Z0-9_]").indexIn(userId) >= 0) {
            session->SendClientError("Illegal character in userId" + userId + ", only use alphanumeric and underscore");
        }
        else if (_sessions.contains(userId)) {
            session->SendClientError("User name is already in use");
        }
        else {
            qDebug() << "Server::DoLogon() User" << userId << "logged in";
            //v1.3 - Note: only assign userId when user name is not already in use
            session->SetId(userId);
            session->SetRoomId(roomId);

            _sessions[userId] = session;

            //send our UDP port as well
            QJsonObject data;
            data["udp"] = _udpsocket->localPort();
            session->SendData("okay", data);
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
            session->SendData("okay");
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
            session->SendData("okay");
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

void Server::DoChat(Session *session, QJsonObject data)
{
//    qDebug() << "Server::DoChat()";
    QJsonObject chatdata;
    chatdata["roomId"] = session->GetRoomId();
    chatdata["userId"] = session->GetId();

    if (data.contains("message")) {
        //broadcast out user_chat event to all users listening to roomId
        chatdata["message"] = data["message"];

        //PM a user, or broadcast to all
        if (data.contains("toUserId")) {
            BroadcastToUser(data["toUserId"].toString(), "user_chat", chatdata);
        }
        else {
            BroadcastToRoom(session, "user_chat", chatdata);
        }
    }
}

void Server::DoPortal(Session *session, QJsonObject data)
{
//    qDebug() << "Server::DoPortal()";

    //broadcast out user_portal event to all users listening to roomId
    data["roomId"] = session->GetRoomId();
    data["userId"] = session->GetId();

    session->SendData("okay");

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

void Server::BroadcastToUser(QString userId, const QString method, const QJsonObject data)
{
    QJsonObject o;
    o["method"] = method;
    o["data"] = data;

    QJsonDocument doc(o);
    QByteArray b = doc.toJson(QJsonDocument::Compact) + "\n";

    const bool udpPreferred = (method == "user_moved");

    QPointer <Session> s;
    if (_sessions.contains(userId) && _sessions[userId]) {
        s = _sessions[userId];
    }

    if (s && s->GetSocket()) {
        s->SendMessage(b, udpPreferred);
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

    const bool udpPreferred = (method == "user_moved");

    QVector <QPointer <Session> > & sessions = _rooms[session->GetRoomId()];
    for (QPointer <Session> & s : sessions) {
        if (s && s->GetSocket() && s != session) {
//            qDebug() << " broadcasting" << method << "to user" << s->GetId();
            s->SendMessage(b, udpPreferred);
        }
    }
}
