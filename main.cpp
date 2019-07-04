/*
 * janus-server-c
 *
 * A server-side console application that is a C++ implementation of the Janus Presence Server.
 *
 * Author: James McCrae, Janus VR, Inc.
 *
 */

#include <QCoreApplication>
#include <QPointer>
#include <QTimer>

#include "server.h"

int main(int argc, char *argv[])
{
    //port defaults
    int wsport = 5566;
    int udpport = 5568;

    //process cmd line args
    for (int i=1; i<argc; ++i) {
        QString arg=argv[i];

        if (arg == "-wsport" && i < argc-1) {
            ++i;
            wsport = QString(argv[i]).toInt();
        }
        else if (arg == "-udpport" && i < argc-1) {
            ++i;
            udpport = QString(argv[i]).toInt();
        }
    }

    //start server
    QCoreApplication a(argc, argv);
    Server s(wsport, udpport);
    return a.exec();
}
