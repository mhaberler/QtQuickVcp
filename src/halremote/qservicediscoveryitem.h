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
#ifndef QAPPDISCOVERYITEM_H
#define QAPPDISCOVERYITEM_H

#include <QQuickItem>
#include <QHostAddress>
#include <QDateTime>

class QServiceDiscoveryItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(QString uuid READ uuid WRITE setUuid NOTIFY uuidChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QHostAddress hostAddress READ hostAddress WRITE setHostAddress NOTIFY hostAddressChanged)
    Q_PROPERTY(QDateTime expiryDate READ expiryDate WRITE setExpiryDate NOTIFY expiryDateChanged)
    Q_PROPERTY(QStringList txtRecords READ txtRecords WRITE setTxtRecords NOTIFY txtRecordsChanged)

public:
    explicit QServiceDiscoveryItem(QQuickItem *parent = 0);


QString uri() const
{
    return m_uri;
}

int port() const
{
    return m_port;
}

QHostAddress hostAddress() const
{
    return m_hostAddress;
}

QString name() const
{
    return m_name;
}

QDateTime expiryDate() const
{
    return m_expiryDate;
}

QString type() const
{
    return m_type;
}

QStringList txtRecords() const
{
    return m_txtRecords;
}

QString uuid() const
{
    return m_uuid;
}

public slots:

void setUri(QString arg)
{
    if (m_uri != arg) {
        m_uri = arg;
        emit uriChanged(arg);
    }
}
void setPort(int arg)
{
    if (m_port != arg) {
        m_port = arg;
        emit portChanged(arg);
    }
}
void setHostAddress(QHostAddress arg)
{
    if (m_hostAddress != arg) {
        m_hostAddress = arg;
        emit hostAddressChanged(arg);
    }
}

void setName(QString arg)
{
    if (m_name != arg) {
        m_name = arg;
        emit nameChanged(arg);
    }
}

void setExpiryDate(QDateTime arg)
{
    if (m_expiryDate != arg) {
        m_expiryDate = arg;
        emit expiryDateChanged(arg);
    }
}

void setType(QString arg)
{
    if (m_type != arg) {
        m_type = arg;
        emit typeChanged(arg);
    }
}

void setTxtRecords(QStringList arg)
{
    if (m_txtRecords != arg) {
        m_txtRecords = arg;
        emit txtRecordsChanged(arg);
    }
}

void setUuid(QString arg)
{
    if (m_uuid != arg) {
        m_uuid = arg;
        emit uuidChanged(arg);
    }
}

private:
    QString m_uri;
    int m_port;
    QHostAddress m_hostAddress;
    QString m_name;
    QDateTime m_expiryDate;
    QString m_type;
    QStringList m_txtRecords;

    QString m_uuid;

signals:

void uriChanged(QString arg);
void portChanged(int arg);
void hostAddressChanged(QHostAddress arg);
void nameChanged(QString arg);
void expiryDateChanged(QDateTime arg);
void typeChanged(QString arg);
void txtRecordsChanged(QStringList arg);
void uuidChanged(QString arg);
};

#endif // QAPPDISCOVERYITEM_H
