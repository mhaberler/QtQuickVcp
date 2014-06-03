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
#include "qhalremotecomponent.h"

/*!
    \qmltype HalRemoteComponent
    \instantiates QHalRemoteComponent
    \inqmlmodule Machinekit.HalRemote
    \brief A HAL remote component.
    \ingroup halremote

    This component provides the counterpart of a HAL
    remote component in the HAL real-time environment.
    The HalRemoteComponent connects to a remote HAL instance
    using the halrcmd and halrcomp services provided by
    a Haltalk instance running on the remote host.

    A HalRemoteComponent needs the \l{halrcmdUri},
    \l{halrcompUri} and \l containerItem set in order
    to work.

    The HalRemoteComponent scans the \l containerItem
    and its children for \l{HalPin}s when \l ready is set
    to \c true.

    The following example creates a HAL remote component
    \c myComponent with one pin \c myPin. The resulting
    name for the pin inside HAL is \c myComponent.myPin.

    \qml
    Item {
        Item {
            id: container

            HalPin {
                name: "myPin"
                type: HalPin.Float
                direction: HalPin.Out
            }
        }

        HalRemoteComponent {
            id: halRemoteComponent

            name: "myComponent"
            halrcmdUri: "tcp://192.168.1.2:5001"
            halrcompUri: "tcp://192.168.1.2:5002"
            containerItem: container
            ready: true
        }
    }
    \endqml
*/

/*! \qmlproperty string HalRemoteComponent::halrcmdUri

    This property holds the halrcmd service uri.
*/

/*! \qmlproperty string HalRemoteComponent::halrcompUri

    This property holds the halrcomp service uri.
*/

/*! \qmlproperty string HalRemoteComponent::name

    This property holds the name of the remote component.
*/

/*! \qmlproperty int HalRemoteComponent::heartbeadPeriod

    This property holds the period time of the heartbeat timer in ms.

    The default value is \c{3000}.
*/

/*! \qmlproperty bool HalRemoteComponent::ready

    This property holds whether the HalRemoteComponent is ready or not.
    If the property is set to \c true the component will try to connect. If the
    property is set to \c false all connections will be closed.

    The default value is \c{false}.
*/

/*! \qmlproperty enumeration HalRemoteComponent::connectionState

    This property holds the connection state of the HAL remote component.

    \list
    \li HalRemoteComponent.Disconnected - The component is not connected.
    \li HalRemoteComponent.Connecting - The component is trying to connect to the remote component.
    \li HalRemoteComponent.Connected - The component is connected and pin changes are accepted.
    \li HalRemoteComponent.Error - An error has happened. See \l error and \l errorString for details about the error.
    \endlist
*/

/*! \qmlproperty enumeration HalRemoteComponent::error

    This property holds the currently active error. See \l errorString
    for a description of the active error.

    \list
    \li HalRemoteComponent.NoError - No error happened.
    \li HalRemoteComponent.BindError - Binding the remote component failed.
    \li HalRemoteComponent.PinChangeError - A pin change was rejected.
    \li HalRemoteComponent.CommandError - A command was rejected.
    \li HalRemoteComponent.TimeoutError - The connection timed out.
    \li HalRemoteComponent.SocketError - An error related to the sockets happened.
    \endlist

    \sa errorString
*/

/*! \qmlproperty string HalRemoteComponent::errorString

    This property holds a text description of the last error that occured.
    If \l error holds a error value check this property for the description.

    \sa error
*/

/*! \qmlproperty Item HalRemoteComponent::containerItem

    This property holds the item that should be scanned for
    \l{HalPin}s.

    The default value is \c{NULL}.
*/

/** Remote HAL Component implementation for use with C++ and QML */
QHalRemoteComponent::QHalRemoteComponent(QQuickItem *parent) :
    QQuickItem(parent),
    m_halrcmdUri(""),
    m_halrcompUri(""),
    m_name("default"),
    m_heartbeatPeriod(3000),
    m_sState(Down),
    m_cState(Down),
    m_connectionState(Disconnected),
    m_error(NoError),
    m_errorString(""),
    m_ready(false),
    m_containerItem(NULL),
    m_componentCompleted(false),
    m_context(NULL),
    m_halrcompSocket(NULL),
    m_halrcmdSocket(NULL),
    m_heartbeatTimer(new QTimer(this))
{
    connect(m_heartbeatTimer, SIGNAL(timeout()),
            this, SLOT(hearbeatTimerTick()));
}

QHalRemoteComponent::~QHalRemoteComponent()
{
    disconnectSockets();
}

/** componentComplete is executed when the QML component is fully loaded */
void QHalRemoteComponent::componentComplete()
{
    m_componentCompleted = true;

    if (m_ready == true)    // the component was set to ready before it was completed
    {
        start();
    }

    QQuickItem::componentComplete();
}

/** Scans all children of the container item for pins and adds them to a map */
void QHalRemoteComponent::addPins()
{
    QObjectList halObjects;

    if (m_containerItem == NULL)
    {
        return;
    }

    halObjects = recurseObjects(m_containerItem->children());
    foreach (QObject *object, halObjects)
    {
        QHalPin *pin = static_cast<QHalPin *>(object);
        if (pin->name().isEmpty()  || (pin->enabled() == false))    // ignore pins with empty name and disabled pins
        {
            continue;
        }
        m_pinsByName[pin->name()] = pin;
        connect(pin, SIGNAL(valueChanged(QVariant)),
                this, SLOT(pinChange(QVariant)));
#ifdef QT_DEBUG
        qDebug() << "pin added: " << pin->name();
#endif
    }
}

/** Removes all previously added pins */
void QHalRemoteComponent::removePins()
{
    foreach (QHalPin *pin, m_pinsByName)
    {
        disconnect(pin, SIGNAL(valueChanged(QVariant)),
                this, SLOT(pinChange(QVariant)));
    }

    m_pinsByHandle.clear();
    m_pinsByName.clear();
}

/** Connects the 0MQ sockets */
bool QHalRemoteComponent::connectSockets()
{
    m_context = createDefaultContext(this);
    m_context->start();

    m_halrcmdSocket = m_context->createSocket(ZMQSocket::TYP_DEALER, this);
    m_halrcmdSocket->setLinger(0);
    m_halrcmdSocket->setIdentity(QString("%1-%2").arg(m_name).arg(QCoreApplication::applicationPid()).toLocal8Bit());


    m_halrcompSocket = m_context->createSocket(ZMQSocket::TYP_SUB, this);
    m_halrcompSocket->setLinger(0);

    try {
        m_halrcmdSocket->connectTo(m_halrcmdUri);
        m_halrcompSocket->connectTo(m_halrcompUri);
    }
    catch (zmq::error_t e) {
        QString errorString;
        errorString = QString("Error %1: ").arg(e.num()) + QString(e.what());
        updateError(SocketError, errorString);
        updateState(Error);
        return false;
    }

    connect(m_halrcompSocket, SIGNAL(messageReceived(QList<QByteArray>)),
            this, SLOT(updateMessageReceived(QList<QByteArray>)));
    connect(m_halrcmdSocket, SIGNAL(messageReceived(QList<QByteArray>)),
            this, SLOT(cmdMessageReceived(QList<QByteArray>)));

#ifdef QT_DEBUG
    qDebug() << "sockets connected" << m_halrcompUri << m_halrcmdUri;
#endif

    return true;
}

/** Disconnects the 0MQ sockets */
void QHalRemoteComponent::disconnectSockets()
{
    if (m_halrcmdSocket != NULL)
    {
        m_halrcmdSocket->close();
        m_halrcmdSocket->deleteLater();
        m_halrcmdSocket = NULL;
    }

    if (m_halrcompSocket != NULL)
    {
        m_halrcompSocket->close();
        m_halrcompSocket->deleteLater();
        m_halrcompSocket = NULL;
    }

    if (m_context != NULL)
    {
        m_context->stop();
        m_context->deleteLater();
        m_context = NULL;
    }
}

/** Generates a Bind messages and sends it over the suitable 0MQ socket */
void QHalRemoteComponent::bind()
{
    pb::Component *component;

    m_tx.set_type(pb::MT_HALRCOMP_BIND);
    component = m_tx.add_comp();
    component->set_name(m_name.toStdString());
    foreach (QHalPin *pin, m_pinsByName)
    {
        pb::Pin *halPin = component->add_pin();
        halPin->set_name(QString("%1.%2").arg(m_name).arg(pin->name()).toStdString());  // pin name is always component.name
        halPin->set_type((pb::ValueType)pin->type());
        halPin->set_dir((pb::HalPinDirection)pin->direction());
        if (pin->type() == QHalPin::Float)
        {
            halPin->set_halfloat(pin->value().toDouble());
        }
        else if (pin->type() == QHalPin::Bit)
        {
            halPin->set_halbit(pin->value().toBool());
        }
        else if (pin->type() == QHalPin::S32)
        {
            halPin->set_hals32(pin->value().toInt());
        }
        else if (pin->type() == QHalPin::U32)
        {
            halPin->set_halu32(pin->value().toUInt());
        }
    }

#ifdef QT_DEBUG
    std::string s;
    gpb::TextFormat::PrintToString(m_tx, &s);
    qDebug() << "bind:" << QString::fromStdString(s);
#endif

    m_halrcmdSocket->sendMessage(QByteArray(m_tx.SerializeAsString().c_str(), m_tx.ByteSize()));
    m_tx.Clear();
}

/** Updates a local pin with the value of a remote pin */
void QHalRemoteComponent::pinUpdate(pb::Pin remotePin, QHalPin *localPin)
{
#ifdef QT_DEBUG
    qDebug() << "pinupdate" << localPin->name();
#endif

    if (remotePin.has_halfloat())
    {
        localPin->setValue(QVariant(remotePin.halfloat()), true);
    }
    else if (remotePin.has_halbit())
    {
        localPin->setValue(QVariant(remotePin.halbit()), true);
    }
    else if (remotePin.has_hals32())
    {
        localPin->setValue(QVariant(remotePin.hals32()), true);
    }
    else if (remotePin.has_halu32())
    {
        localPin->setValue(QVariant(remotePin.halu32()), true);
    }
}

/** Updates a remote pin witht the value of a local pin */
void QHalRemoteComponent::pinChange(QVariant value)
{
    Q_UNUSED(value)
    QHalPin *pin;
    pb::Pin *halPin;

#ifdef QT_DEBUG
    qDebug() << "pinchange" << (m_connectionState == Connected);
#endif

    if (m_connectionState != Connected) // only accept pin changes if we are connected
    {
        return;
    }

    pin = static_cast<QHalPin *>(QObject::sender());

    if (pin->direction() == QHalPin::In)   // Only update IN or IO pins
    {
        return;
    }

    m_tx.set_type(pb::MT_HALRCOMP_SET);

    // This message MUST carry a Pin message for each pin which has
    // changed value since the last message of this type.
    // Each Pin message MUST carry the handle field.
    // Each Pin message MAY carry the name field.
    // Each Pin message MUST - depending on pin type - carry a halbit,
    // halfloat, hals32, or halu32 field.
    halPin = m_tx.add_pin();

    halPin->set_handle(pin->handle());
    halPin->set_name(QString("%1.%2").arg(m_name).arg(pin->name()).toStdString());
    halPin->set_type((pb::ValueType)pin->type());
    if (pin->type() == QHalPin::Float)
    {
        halPin->set_halfloat(pin->value().toDouble());
    }
    else if (pin->type() == QHalPin::Bit)
    {
        halPin->set_halbit(pin->value().toBool());
    }
    else if (pin->type() == QHalPin::S32)
    {
        halPin->set_hals32(pin->value().toInt());
    }
    else if (pin->type() == QHalPin::U32)
    {
        halPin->set_halu32(pin->value().toUInt());
    }

    m_halrcmdSocket->sendMessage(QByteArray(m_tx.SerializeAsString().c_str(), m_tx.ByteSize()));
    m_tx.Clear();
}

void QHalRemoteComponent::start()
{
#ifdef QT_DEBUG
            qDebug() << "ready" << m_name;
#endif
    m_cState = Trying;
    updateState(Connecting);


    if (connectSockets())
    {
        addPins();
        bind();
    }
}

void QHalRemoteComponent::stop()
{
#ifdef QT_DEBUG
            qDebug() << "stop" << m_name;
#endif

    // cleanup here
    stopHeartbeat();
    disconnectSockets();
    removePins();

    updateState(Disconnected);
    updateError(NoError, "");   // clear the error here
}

void QHalRemoteComponent::startHeartbeat()
{
    m_heartbeatTimer->setInterval(m_heartbeatPeriod);
    m_heartbeatTimer->start();
}

void QHalRemoteComponent::stopHeartbeat()
{
    m_heartbeatTimer->stop();
}

void QHalRemoteComponent::updateState(State state)
{
    if (state != m_connectionState)
    {
        m_connectionState = state;
        emit connectionStateChanged(m_connectionState);

        if (m_connectionState == Connected)
        {
            startHeartbeat();
        }
        else
        {
            stopHeartbeat();
        }
    }
}

void QHalRemoteComponent::updateError(QHalRemoteComponent::ConnectionError error, QString errorString)
{
    m_error = error;
    m_errorString = errorString;

    emit errorStringChanged(m_errorString);
    emit errorChanged(m_error);
}

/** If the ready property has a rising edge we try to connect
 *  if it is has a falling edge we disconnect and cleanup
 */
void QHalRemoteComponent::setReady(bool arg)
{
    if (m_ready != arg) {
        m_ready = arg;
        emit readyChanged(arg);

        if (m_componentCompleted == false)
        {
            return;
        }

        if (m_ready)
        {
            start();
        }
        else
        {
            stop();
        }
    }
}

/** Recurses through a list of objects */
QObjectList QHalRemoteComponent::recurseObjects(const QObjectList &list)
{
    QObjectList halObjects;

    foreach (QObject *object, list)
    {
        if (object->inherits("QHalPin"))
        {
            halObjects.append(object);
        }

        if (object->children().size() > 0)
        {
            halObjects.append(recurseObjects(object->children()));
        }
    }

    return halObjects;
}

/** Processes all message received on the update 0MQ socket */
void QHalRemoteComponent::updateMessageReceived(QList<QByteArray> messageList)
{
    QByteArray topic;

    topic = messageList.at(0);
    m_rx.ParseFromArray(messageList.at(1).data(), messageList.at(1).size());

#ifdef QT_DEBUG
    qDebug() << "status update" << topic << QString::fromStdString(m_rx.SerializeAsString());
#endif

    if (m_rx.type() == pb::MT_HALRCOMP_INCREMENTAL_UPDATE) //incremental update
    {
        for (int i = 0; i < m_rx.pin_size(); ++i)
        {
            pb::Pin remotePin = m_rx.pin(i);
            QHalPin *localPin = m_pinsByHandle[remotePin.handle()];
            pinUpdate(remotePin, localPin);
        }

        if (m_sState != Up)
        {
            m_sState = Up;
            updateError(NoError, "");
            updateState(Connected);
        }

        return;
    }
    else if (m_rx.type() == pb::MT_HALRCOMP_FULL_UPDATE)
    {
        for (int i = 0; i < m_rx.comp_size(); ++i)
        {
            pb::Component component = m_rx.comp(i);
            for (int j = 0; j < component.pin_size(); j++)
            {
                pb::Pin remotePin = component.pin(j);
                QString name = QString::fromStdString(remotePin.name());
                if (name.indexOf(".") != -1)    // strip comp prefix
                {
                    name = name.mid(name.indexOf(".")+1);
                }
                QHalPin *localPin = m_pinsByName[name];
                localPin->setHandle(remotePin.handle());
                m_pinsByHandle[remotePin.handle()] = localPin;
                pinUpdate(remotePin, localPin);
            }

            if (m_sState != Up)
            {
                m_sState = Up;
                updateError(NoError, "");
                updateState(Connected);
            }
        }

        return;
    }
    else if (m_rx.type() == pb::MT_HALRCOMMAND_ERROR)
    {
        QString errorString;

        for (int i = 0; i < m_rx.note_size(); ++i)
        {
            errorString.append(QString::fromStdString(m_rx.note(i)) + "\n");
        }

        m_sState = Down;
        updateError(CommandError, errorString);
        updateState(Error);

#ifdef QT_DEBUG
        qDebug() << "proto error on subscribe" << errorString;
#endif

        return;
    }

#ifdef QT_DEBUG
        qDebug() << "status_update: unknown message type: " << QString::fromStdString(m_rx.SerializeAsString());
#endif
}

/** Processes all message received on the command 0MQ socket */
void QHalRemoteComponent::cmdMessageReceived(QList<QByteArray> messageList)
{
    m_rx.ParseFromArray(messageList.at(0).data(), messageList.at(0).size());

#ifdef QT_DEBUG
    std::string s;
    gpb::TextFormat::PrintToString(m_rx, &s);
    qDebug() << "server message" << QString::fromStdString(s);
#endif

    if (m_rx.type() == pb::MT_PING_ACKNOWLEDGE)
    {
        m_cState = Up;
        m_pingOutstanding = false;

        if (m_sState == Up)
        {
            updateError(NoError, "");
            updateState(Connected);
        }

        return;
    }
    else if (m_rx.type() == pb::MT_HALRCOMP_BIND_CONFIRM)
    {
#ifdef QT_DEBUG
        qDebug() << "bind confirmed";
#endif
        m_cState = Up;
        m_sState = Trying;
        m_halrcompSocket->subscribeTo(m_name.toLocal8Bit());

        return;
    }
    else if ((m_rx.type() == pb::MT_HALRCOMP_BIND_REJECT)
             || (m_rx.type() == pb::MT_HALRCOMP_SET_REJECT))
    {
        QString errorString;

        for (int i = 0; i < m_rx.note_size(); ++i)
        {
            errorString.append(QString::fromStdString(m_rx.note(i)) + "\n");
        }

        m_cState = Down;

        if (m_rx.type() == pb::MT_HALRCOMP_BIND_REJECT)
        {
            updateError(BindError, errorString);
        }
        else
        {
            updateError(PinChangeError, errorString);
        }
        updateState(Error);


#ifdef QT_DEBUG
        if (m_rx.type() == pb::MT_HALRCOMP_BIND_REJECT) {
            qDebug() << "bind rejected" << errorString;
        }
        else {
            qDebug() << "pin change rejected" << QString::fromStdString(m_rx.note(0));
        }
#endif
        return;
    }
    else
    {
#ifdef QT_DEBUG
        qDebug() << "UNKNOWN server message type";
#endif
    }
}

void QHalRemoteComponent::hearbeatTimerTick()
{
    if (m_pingOutstanding)
    {
        m_cState = Trying;
        updateError(TimeoutError, "Remote host timed out");
        updateState(Error);

#ifdef QT_DEBUG
        qDebug() << "timeout";
#endif
    }

    m_tx.set_type(pb::MT_PING);

    m_halrcmdSocket->sendMessage(QByteArray(m_tx.SerializeAsString().c_str(), m_tx.ByteSize()));
    m_tx.Clear();

    m_pingOutstanding = true;
}

