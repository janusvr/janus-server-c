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
    QCoreApplication a(argc, argv);
    Server s;
    return a.exec();
}
