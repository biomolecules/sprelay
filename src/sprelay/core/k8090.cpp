/***************************************************************************
**                                                                        **
**  Controlling interface for K8090 8-Channel Relay Card from Velleman    **
**  through usb using virtual serial port in Qt.                          **
**  Copyright (C) 2017 Jakub Klener                                       **
**                                                                        **
**  This file is part of SpRelay application.                             **
**                                                                        **
**  You can redistribute it and/or modify it under the terms of the       **
**  3-Clause BSD License as published by the Open Source Initiative.      **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          **
**  3-Clause BSD License for more details.                                **
**                                                                        **
**  You should have received a copy of the 3-Clause BSD License along     **
**  with this program.                                                    **
**  If not, see https://opensource.org/licenses/                          **
**                                                                        **
****************************************************************************/

/*!
    \file k8090.h
*/

#include "k8090.h"

#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringBuilder>


namespace sprelay {
namespace core {

/*!
    \namespace K8090Traits
    \brief Contains traits for K8090 class.
*/
using namespace K8090Traits;  // NOLINT(build/namespaces)

/*!
    \enum Command
    \brief Scoped enumeration listing all commands.

    See the Velleman %K8090 card manual.
*/
/*!
    \var Command::RELAY_ON
    \brief Switch realy on command.
*/
/*!
    \var Command::RELAY_OFF
    \brief Switch realy off command.
*/
/*!
    \var Command::TOGGLE_RELAY
    \brief Toggle realy command.
*/
/*!
    \var Command::QUERY_RELAY
    \brief Query relay status command.
*/
/*!
    \var Command::SET_BUTTON_MODE
    \brief Set button mode command.
*/
/*!
    \var Command::BUTTON_MODE
    \brief Query button mode command.
*/
/*!
    \var Command::START_TIMER
    \brief Start relay timer command.
*/
/*!
    \var Command::SET_TIMER
    \brief Set relay timer delay command.
*/
/*!
    \var Command::TIMER
    \brief Query timer delay command.
*/
/*!
    \var Command::BUTTON_STATUS
    \brief Button status command.
*/
/*!
    \var Command::RELAY_STATUS
    \brief Relay status command.
*/
/*!
    \var Command::RESET_FACTORY_DEFAULTS
    \brief Reset factory defaults command.
*/
/*!
    \var Command::JUMPER_STATUS
    \brief Jumper status command.
*/
/*!
    \var Command::FIRMWARE_VERSION
    \brief Firmware version command.
*/
/*!
    \var Command::NONE
    \brief The number of all commands represents also none command.
*/

/*!
    \enum RelayID
    \brief Scoped enumeration listing all 8 relays.

    Bitwise operators are enabled for this enum by overloading K8090Traits::enable_bitmask_operators(RelayID) function
    (see enum_flags.h for more details) and so the value of K8090Traits::RelayID type can be also a combination of
    particular relays.
*/
/*!
    \var RelayID::NONE
    \brief None relay.
*/
/*!
    \var RelayID::ONE
    \brief First relay.
*/
/*!
    \var RelayID::TWO
    \brief Second relay.
*/
/*!
    \var RelayID::THREE
    \brief Third relay.
*/
/*!
    \var RelayID::FOUR
    \brief Fourth relay.
*/
/*!
    \var RelayID::FIVE
    \brief Fifth relay.
*/
/*!
    \var RelayID::SIX
    \brief Sixth relay.
*/
/*!
    \var RelayID::SEVEN
    \brief Seventh relay.
*/
/*!
    \var RelayID::EIGHT
    \brief Eigth relay.
*/
/*!
    \var RelayID::ALL
    \brief All relays.
*/

/*!
    \fn constexpr bool enable_bitmask_operators(RelayID)
    \brief Function overload which enables bitwise operators for RelayID enumeration. See enum_flags.h for more
    details.

    \return True to enable bitmask operators.
*/

/*!
    \fn constexpr std::underlying_type<E> as_number(const E e)
    \brief Converts enumeration to its underlying type.

    \param e Enumerator to be converted.
    \return The enum value as underlying type.
*/

// static constants
const quint16 K8090::kProductID = 32912;
const quint16 K8090::kVendorID = 4303;

/*!
    \brief Start delimiting byte.
*/
const unsigned char kStxByte_ = 0x04;
/*!
    \brief End delimiting byte.
*/
const unsigned char kEtxByte_ = 0x0f;

// generate static array containing commands at compile time

namespace {  // unnamed namespace

// template function to fill the array with appropriate commands
template<unsigned int N>
constexpr unsigned char getXDataValue();

// specializations
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::RELAY_ON)>()
{
    return 0x11;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::RELAY_OFF)>()
{
    return 0x12;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::TOGGLE_RELAY)>()
{
    return 0x14;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::QUERY_RELAY)>()
{
    return 0x18;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::SET_BUTTON_MODE)>()
{
    return 0x21;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::BUTTON_MODE)>()
{
    return 0x22;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::START_TIMER)>()
{
    return 0x41;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::SET_TIMER)>()
{
    return 0x42;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::TIMER)>()
{
    return 0x44;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::BUTTON_STATUS)>()
{
    return 0x50;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::RELAY_STATUS)>()
{
    return 0x51;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::RESET_FACTORY_DEFAULTS)>()
{
    return 0x66;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::JUMPER_STATUS)>()
{
    return 0x70;
}
template<>
constexpr unsigned char getXDataValue<as_number(CommandID::FIRMWARE_VERSION)>()
{
    return 0x71;
}


// Template containing static array
template<unsigned char ...Args>
struct XArrayData
{
    static const unsigned char kValues[sizeof...(Args)];
};

// recursively generates XData typedef
template<unsigned int N, unsigned char ...Args>
struct XArrayGenerator_
{
    typedef typename XArrayGenerator_<N - 1, getXDataValue<N - 1>(), Args...>::XData XData;
};

// end case template partial specialization
template<unsigned char ...Args>
struct XArrayGenerator_<1u, Args...>
{
    typedef XArrayData<getXDataValue<0u>(), Args...> XData;
};

// XArray generates recursively XData type, which contains static constant array kValues.
// Usage: unsigned char arr = XArray<K8090Traits::Comand::None>::XData::kValues
template<unsigned char N>
struct XArray
{
    typedef typename XArrayGenerator_<N>::XData XData;
};

// static const array initialization
template<unsigned char ...Args>
const unsigned char XArrayData<Args...>::kValues[sizeof...(Args)] = {Args...};

}  // unnamed namespace


/*!
    \class K8090
    \brief The class that provides the interface for Velleman %K8090 relay card
    controlling through serial port.

    \remark reentrant, thread-safe
*/

// initialization of static member variables
/*!
    \brief Array of hexadecimal representation of commands used to control the relay.

    They should be accessed using the K8090Traits::Command enum values.

    Example, which shows how to build the whole command read status:

    \code
    int n = 7;  // Number of command bytes.
    unsigned char cmd[n]; // array of command bytes

    // copying the first two bytes of command to command byte array
    cmd[0] = kStxByte_
    cmd[1] = commands_[as_number(Command::RELAY_STATUS)];
    cmd[2] = as_number(RelayID::ONE);  // 3rd byte specifies affected relays
    // commands, there is no one.
    cmd[5] = K8090::checkSum(cmd, 5); // sixth byte contains check sum.
    cmd[6] = kEtxByte_;
    \endcode
*/
const unsigned char *K8090::commands_ = XArray<as_number(CommandID::NONE)>::XData::kValues;


/*!
  \brief Creates a new K8090 instance and sets the default values.
*/
K8090::K8090(QObject *parent) :
    QObject(parent)
{
    connected_ = false;

    serial_port_ = new QSerialPort(this);
    connect(serial_port_, &QSerialPort::readyRead, this, &K8090::onReadyData);
}


QList<ComPortParams> K8090::availablePorts()
{
    QList<ComPortParams> comPortParamsList;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {  // NOLINT(whitespace/parens)
        ComPortParams comPortParams;
        comPortParams.portName = info.portName();
        comPortParams.description = info.description();
        comPortParams.manufacturer = info.manufacturer();
        comPortParams.productIdentifier = info.productIdentifier();
        comPortParams.vendorIdentifier = info.vendorIdentifier();
        comPortParamsList.append(comPortParams);
    }
    return comPortParamsList;
}


void K8090::setComPortName(const QString &name)
{
    if (com_port_name_ != name) {
        com_port_name_ = name;
        connected_ = false;
        serial_port_->close();
    }
}

bool K8090::isConnected()
{
    return connected_;
}


void K8090::connectK8090()
{
    connected_ = false;
    bool cardFound = false;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {  // NOLINT(whitespace/parens)
        if (info.portName() == com_port_name_
                && info.productIdentifier() == kProductID
                && info.vendorIdentifier() == kVendorID) {
            cardFound = true;
        }
    }

    if (!cardFound) {
        emit connectionFailed();
        return;
    }

    serial_port_->setPortName(com_port_name_);
    serial_port_->setBaudRate(QSerialPort::Baud19200);
    serial_port_->setDataBits(QSerialPort::Data8);
    serial_port_->setParity(QSerialPort::NoParity);
    serial_port_->setStopBits(QSerialPort::OneStop);
    serial_port_->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial_port_->isOpen()) {
        if (!serial_port_->open(QIODevice::ReadWrite)) {
            emit connectionFailed();
            return;
        }
    }

    connected_ = true;
    sendCommandHelper(CommandID::QUERY_RELAY);
    sendCommandHelper(CommandID::BUTTON_MODE);
    sendCommandHelper(CommandID::TIMER, RelayID::ALL, as_number(TimerDelayType::TOTAL));
    sendCommandHelper(CommandID::JUMPER_STATUS);
    sendCommandHelper(CommandID::FIRMWARE_VERSION);
}

void K8090::disconnect()
{
    connected_ = false;
    serial_port_->close();
}

void K8090::refreshRelaysInfo()
{
    sendCommandHelper(CommandID::QUERY_RELAY);
    sendCommandHelper(CommandID::BUTTON_MODE);
    sendCommandHelper(CommandID::TIMER, RelayID::ALL, as_number(TimerDelayType::TOTAL));
    sendCommandHelper(CommandID::JUMPER_STATUS);
    sendCommandHelper(CommandID::FIRMWARE_VERSION);
}

void K8090::switchRelayOn(RelayID relays)
{
    sendCommand(CommandID::RELAY_ON, relays);
}

void K8090::switchRelayOff(RelayID relays)
{
    sendCommand(CommandID::RELAY_OFF, relays);
}

void K8090::toggleRelay(RelayID relays)
{
    sendCommand(CommandID::TOGGLE_RELAY, relays);
}

void K8090::setButtonMode(RelayID momentary, RelayID toggle, RelayID timed)
{
    sendCommand(CommandID::SET_BUTTON_MODE, momentary, as_number(toggle), as_number(timed));
}

void K8090::startRelayTimer(RelayID relays, quint16 delay)
{
    sendCommand(CommandID::START_TIMER, relays, highByte(delay), lowByte(delay));
}

void K8090::setRelayTimerDelay(RelayID relays, quint16 delay)
{
    sendCommand(CommandID::SET_TIMER, relays, highByte(delay), lowByte(delay));
}

void K8090::queryRelayStatus()
{
    sendCommand(CommandID::RELAY_STATUS);
}

void K8090::queryRemainingTimerDelay(RelayID relays)
{
    sendCommand(CommandID::TIMER, relays, as_number(TimerDelayType::CURRENT));
}

void K8090::queryTotalTimerDelay(RelayID relays)
{
    sendCommand(CommandID::TIMER, relays, as_number(TimerDelayType::TOTAL));
}

void K8090::queryButtonModes()
{
    sendCommand(CommandID::BUTTON_MODE);
}

void K8090::resetFactoryDefaults()
{
    sendCommand(CommandID::RESET_FACTORY_DEFAULTS);
}

void K8090::queryJumperStatus()
{
    sendCommand(CommandID::JUMPER_STATUS);
}

void K8090::queryFirmwareVersion()
{
    sendCommand(CommandID::FIRMWARE_VERSION);
}

void K8090::sendCommand(CommandID command, RelayID mask, unsigned char param1, unsigned char param2)
{
    if (!connected_) {
        emit notConnected();
        return;
    }
    sendCommandHelper(command, mask, param1, param2);
}


void K8090::onReadyData()
{
    qDebug() << "R8090::onReadyData";

    // converting the data to unsigned char
    QByteArray data = serial_port_->readAll();
    int n = data.size();
    unsigned char *buffer = reinterpret_cast<unsigned char*>(data.data());
    qDebug() << byteToHex(buffer, n);
}


void K8090::sendCommandHelper(CommandID command, RelayID mask, unsigned char param1, unsigned char param2)
{
    qDebug() << "K8090::sendCommandHelper()";
    static const int n = 7;  // Number of command bytes.
    std::unique_ptr<unsigned char []> cmd = std::unique_ptr<unsigned char []>{new unsigned char[n]};
    cmd[0] = kStxByte_;
    cmd[1] = commands_[as_number(command)];
    cmd[2] = as_number(mask);
    cmd[3] = param1;
    cmd[4] = param2;
    cmd[5] = checkSum(cmd.get(), 5);
    cmd[6] = kEtxByte_;
    sendToSerial(std::move(cmd), n);
}


/*!
   \brief K8090::sendToSerial
   \param buffer
   \param n
 */
void K8090::sendToSerial(std::unique_ptr<unsigned char[]> buffer, int n)
{
    qDebug() << byteToHex(buffer.get(), n);
    if (!serial_port_->isOpen()) {
        if (!serial_port_->open(QIODevice::ReadWrite)) {
            connected_ = false;
            emit connectionFailed();
            return;
        }
    }

    serial_port_->write(reinterpret_cast<char*>(buffer.release()), n);
}


/*!
   \brief K8090::hexToByte
   \param pbuffer
   \param n
   \param msg
 */
void K8090::hexToByte(unsigned char **pbuffer, int *n, const QString &msg)
{
    // remove white spaces
    QString newMsg = msg;
    newMsg.remove(' ');

    int msgSize = newMsg.size();

    // test correct size of msg, all hex codes consit of 2 characters
    if (msgSize % 2) {
        *pbuffer = nullptr;
        *n = 0;
    } else {
        *n = msgSize / 2;
        *pbuffer = new unsigned char[*n];
        bool ok;
        for (int ii = 0; ii < *n; ++ii) {
            (*pbuffer)[ii] = newMsg.midRef(2 * ii, 2).toUInt(&ok, 16);
        }
    }
}


/*!
   \brief K8090::byteToHex
   \param buffer
   \param n
   \return
 */
QString K8090::byteToHex(const unsigned char *buffer, int n)
{
    QString msg;
    for (int ii = 0; ii < n - 1; ++ii) {
        msg.append(QString("%1").arg((unsigned int)buffer[ii], 2, 16, QChar('0')).toUpper()).append(' ');
    }
    if (n > 0) {
        msg.append(QString("%1").arg((unsigned int)buffer[n - 1], 2, 16, QChar('0')).toUpper());
    }
    return msg;
}


/*!
   \brief K8090::checkSum
   \param msg
   \return
 */
QString K8090::checkSum(const QString &msg)
{
    unsigned char *bMsg;
    int n;
    hexToByte(&bMsg, &n, msg);

    unsigned char bChk = checkSum(bMsg, n);

    delete[] bMsg;

    return QString("%1").arg((unsigned int)bChk, 2, 16, QChar('0')).toUpper();
}


/*!
   \brief K8090::checkSum
   \param bMsg
   \param n
   \return
 */
unsigned char K8090::checkSum(const unsigned char *bMsg, int n)
{
    unsigned int iSum = 0u;
    for (int ii = 0; ii < n; ++ii) {
        iSum += (unsigned int)bMsg[ii];
    }
    unsigned char byteSum = iSum % 256;
    iSum = (unsigned int) (~byteSum) + 1u;
    byteSum = (unsigned char) iSum % 256;

    return byteSum;
}


/*!
   \brief K8090::validateResponse
   \param msg
   \param cmd
   \return
 */
bool K8090::validateResponse(const QString &msg, CommandID cmd)
{
    unsigned char *bMsg;
    int n;
    hexToByte(&bMsg, &n, msg);
    if (validateResponse(bMsg, n, cmd)) {
        delete[] bMsg;
        return true;
    } else {
        delete[] bMsg;
        return false;
    }
}


/*!
    \brief K8090::validateResponse
    \param bMsg Pointer to field of bytes containing the response.
    \param n    The number of command bytes.
    \param cmd  The last command enum value.
    \return     true if response is valid, false if not.
*/
bool K8090::validateResponse(const unsigned char *bMsg, int n, CommandID cmd)
{
    if (n >= 6) {
        if (bMsg[0] != kStxByte_)
            return false;
        if (bMsg[1] != commands_[as_number(cmd)])
            return false;
        unsigned char bTest[5];
        for (int ii = 0; ii < 5; ++ii)
            bTest[ii] = bMsg[ii];
        unsigned char bChkSum = checkSum(bTest, 5);
        if ((unsigned int)bChkSum == (unsigned int)bMsg[5])
            return true;
    }
    return false;
}


/*!
   \brief K8090::~K8090
 */
K8090::~K8090()
{
    serial_port_->close();
    delete serial_port_;
}

}  // namespace core
}  // namespace sprelay
