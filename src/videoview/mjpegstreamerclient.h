/****************************************************************************
**
** Copyright (C) 2014 Alexander Rössler
** License: LGPL version 2.1
**
** This file is part of QtQuickVcp.
**
** All rights reserved. This program and the accompanying materials
** are made available under the terms of the GNU Lesser General Public License
** (LGPL) version 2.1 which accompanies this distribution, and is available at
** http://www.gnu.org/licenses/lgpl-2.1.html
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Lesser General Public License for more details.
**
** Contributors:
** Alexander Rössler @ The Cool Tool GmbH <mail DOT aroessler AT gmail DOT com>
**
****************************************************************************/
#ifndef MJPEGSTREAMER2_H
#define MJPEGSTREAMER2_H

#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QQueue>
#include <QTime>
#include <nzmqt/nzmqt.hpp>
#include "package.pb.h"
#include <google/protobuf/text_format.h>

#if defined(Q_OS_IOS)
namespace gpb = google_public::protobuf;
#else
namespace gpb = google::protobuf;
#endif

using namespace nzmqt;

class MjpegStreamerClient : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(bool ready READ isReady WRITE setReady NOTIFY readyChanged)
    Q_PROPERTY(double timestamp READ timestamp NOTIFY timestampChanged)
    Q_PROPERTY(QTime time READ time NOTIFY timeChanged)
public:
    typedef struct {
        QImage image;
        double timestamp;
        QTime  time;
    } StreamBufferItem;

    explicit MjpegStreamerClient(QQuickPaintedItem *parent = 0);
    ~MjpegStreamerClient();
    virtual void componentComplete();
    void paint(QPainter* painter);

    QString uri() const
    {
        return m_url;
    }

    bool isReady() const
    {
        return m_running;
    }

    int fps() const
    {
        return m_fps;
    }

    double timestamp() const
    {
        return m_timestamp;
    }

    QTime time() const
    {
        return m_time;
    }

signals:
    void uriChanged(QString arg);
    void readyChanged(bool arg);
    void fpsChanged(double arg);
    void timestampChanged(double arg);
    void timeChanged(QTime arg);

public slots:

void setUri(QString arg)
{
    if (m_url != arg) {
        m_url = arg;
        emit uriChanged(arg);
    }
}

void setReady(bool arg);

private:
    StreamBufferItem m_currentStreamBufferItem;
    QQueue<StreamBufferItem> m_streamBuffer;
    QImage      m_frameImg;
    QTimer     *m_framerateTimer;
    QTimer     *m_streamBufferTimer;
    ZMQContext *m_context;
    ZMQSocket  *m_updateSocket;
    pb::Package m_rx; // more efficient to reuse a protobuf Message
    bool        m_componentCompleted;

    QString m_url;
    bool m_running;
    int m_fps;
    int m_frameCount;
    double m_timestamp;

    QTime m_time;

private slots:
    void start();
    void stop();
    void connectSocket();
    void disconnectSocket();
    void updateMessageReceived(QList<QByteArray> messageList);

    void updateFramerate();
    void updateStreamBuffer();
    void updateStreamBufferItem();
};

#endif // MJPEGSTREAMER2_H
