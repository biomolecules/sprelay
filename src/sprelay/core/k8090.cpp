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

#include "k8090.h"

#include <QSerialPort>
#include <QSerialPortInfo>

#include <QStringBuilder>

#include <QDebug>

/*!
    \file k8090.h
*/

/*!
    \namespace K8090Traits
    \brief Contains K8090 traits
*/
using namespace K8090Traits;  // NOLINT(build/namespaces)

// static constants
const quint16 K8090::productID = 32912;
const quint16 K8090::vendorID = 4303;

const QString K8090::stxByte = "04";
const QString K8090::etxByte = "0F";
const QString K8090::switchRelayOnCmd = "11";
const QString K8090::switchRelayOffCmd = "12";
const QString K8090::toggleRelayCmd = "14";
const QString K8090::queryRelayStatusCmd = "18";
const QString K8090::setButtonModeCmd = "21";
const QString K8090::queryButtonModeCmd = "22";
const QString K8090::startRelayTimerCmd = "41";
const QString K8090::setRelayTimerDelayCmd = "42";
const QString K8090::queryTimerDelayCmd = "44";
const QString K8090::buttonStatusCmd = "50";
const QString K8090::relayStatusCmd = "51";
const QString K8090::resetFactoryDefaultsCmd = "66";
const QString K8090::jumperStatusCmd = "70";
const QString K8090::firmwareVersionCmd = "71";

/*!
    \class K8090
    \brief The class that provides the interface for %Velleman K8090 relay card
    controlling through serial port.
    \remark reentrant, thread-safe
*/

// initialization of static member variables
/*!
    \brief Array of 4 byte (3 leading bytes and one command byte) commands used
    to control the relay.
    It is filled by fillCommandsArrays() static method, the first command is
    the command with the most important priority, the last is the least
    important. They should be accessed using the K8090Traits::Command enum
    values.
    Example, which shows how to build the whole command read status:
    \code
    int n = 6; // number of command bytes
    unsigned char cmd[n]; // array of command bytes
    // copying the first two bytes of command to command byte array
    for (int ii = 0; ii < 4; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::ReadStatus)][ii];
    cmd[4] = 0; // 5th byte specifies the number of data bytes. For read
    // commands, there is no one.
    cmd[5] = K8090::checkSum(cmd, 5); // the last byte contains check sum.
    \endcode
*/
unsigned char K8090::bCommands[static_cast<int>(Command::None)][2];

unsigned char K8090::bEtxByte;

/*!
    \brief Array of QString representation of command bytes used to control the
    bath.
    To assemble the full command, it is necessary to prepend the commandBase
    and append byte, which contains number of data bytes, folowed bytes
    containing data and ended with checksum (See CheckSum(const unsigned char,
    int)). It is filled by fillCommandsArrays() static method, the first
    command is the command with the most important priority, the last is the
    least important. They should be accessed using the K8090Traits::Command
    enum values. Example, which shows how to build the whole command Read
    status:
    \code
    QString strCmd(commandBase);
    int n = 0; // number of data bytes
    strCmd.append(strCommands[static_cast<int>(Command::ReadStatus)])
          .append(QString(" %1 ").arg(n, 2, 16, QChar('0')).toUpper());
    strCmd.append(checkSum(strCmd));
    \endcode
*/
QString K8090::strCommands[static_cast<int>(Command::None)];

/*!
  \brief Creates a new K8090 instance and sets the default values.
*/
K8090::K8090(QObject *parent) :
    QObject(parent)
{
    lastCommand = Command::None;
    fillCommandsArrays();

    serialPort_ = new QSerialPort(this);
    connect(serialPort_, &QSerialPort::readyRead, this, &K8090::onReadyData);
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

void K8090::resetusedrelays(int param)
{
    switch (param)
     {
     case 1: used_Relays_ = static_cast<unsigned char>(K8090Traits::RelayID::None); break;
     case 2: used_Relays_2_ = static_cast<unsigned char>(K8090Traits::RelayID::None); break;
     case 3: used_Relays_3_ = static_cast<unsigned char>(K8090Traits::RelayID::None); break;
     default: qDebug() << "Wrong choice";
     }
}

/*!
 * \fn K8090::connectK8090()
 * \brief Function controll if is connected some device and also if is used right device. Then are set port characteristics (port name,
 * baud rate, data bits and others).
 *
 *   In this state are also declared 3 boolen arrays and some commands. If button "Connect" is clicked,
 * function will try to find device and than execute commands. If function don't find device, compiler will message "Card not found!!!"
 */
void K8090::connectK8090()
{
    bool cardFound = false;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {  // NOLINT(whitespace/parens)
        if (info.productIdentifier() == productID &&
                info.vendorIdentifier() == vendorID) {
            cardFound = true;
            comPortName_ = info.portName();
            qDebug() << "Port name: " % comPortName_;
        }
    }

    if (!cardFound) {
        qDebug() << "Card not found!!!";
        return;
    }

    serialPort_->setPortName(comPortName_);
    serialPort_->setBaudRate(QSerialPort::Baud19200);
    serialPort_->setDataBits(QSerialPort::Data8);
    serialPort_->setParity(QSerialPort::NoParity);
    serialPort_->setStopBits(QSerialPort::OneStop);
    serialPort_->setFlowControl(QSerialPort::NoFlowControl);
}

void K8090::add(K8090Traits::RelayID addition, int param)
{
    switch (param)
     {
     case 1: used_Relays_ |= static_cast<unsigned char>(addition); break;
     case 2: used_Relays_2_ |= static_cast<unsigned char>(addition); break;
     case 3: used_Relays_3_ |= static_cast<unsigned char>(addition); break;
     default: qDebug() << "Wrong choice";
     }
}

/*!
 * \fn K8090::onSendToSerial()
 * \param const unsigned char *buffer
 * \param int n
 * \brief Function will send byte array to serial port
 */
void K8090::onSendToSerial(const unsigned char *buffer, int n)
{
    qDebug() << byteToHex(buffer, n);
    if (!serialPort_->isOpen())
        serialPort_->open(QIODevice::ReadWrite);
    serialPort_->write(reinterpret_cast<char*>(const_cast<unsigned char*>(buffer)), n);
    delete[] buffer;
}
/*!
 * \fn K8090::onReadyData()
 * \brief Function will catch data from relay card.
 */
void K8090::onReadyData()
{
    qDebug() << "R8090::onReadyData";

    // converting the data to unsigned char
    QByteArray data = serialPort_->readAll();
    int n = data.size();
    unsigned char *buffer = reinterpret_cast<unsigned char*>(data.data());
    qDebug() << byteToHex(buffer, n);

    lastCommand = Command::None;
}


/*!
  *\fn K8090::sendCommand(int param)
  *\param int param
  *\brief Function will send according to parameter corresponding command. Function is overloaded.
*/
void K8090::sendCommand(int param)
{
    switch (param)
     {
     case 18: sendQueryRelayStatus(); break;
     case 21: sendQueryButtonMode(); break;
     case 66: sendRessetfactorydefaults(); break;
     case 70: sendJumperStatus(); break;
     case 71: sendFirmwareVersion(); break;
     default: qDebug() << "Wrong choice";
     }
}
/*!
  *\fn K8090::sendCommand(bool Relays[8], int param)
  *\param int param
  *\param bool Realys[8]
  *\brief Function will send according to parameter corresponding command. Function is overloaded.
  * Function is using method choose, which makes from boolen array byte.
*/
void K8090::sendCommand(unsigned char used_relays_, int param)
{
    switch (param)
     {
      case 11: sendSwitchRelayOnCommand(used_relays_); break;
      case 12: sendSwitchRelayOffCommand(used_relays_); break;
      case 14: sendtoggleRelayCommand(used_relays_); break;
      default: qDebug() << "Wrong choice";
      }
}
/*!
  *\fn K8090::sendCommand(bool Relays[8], int param, unsigned int Time)
  *\param int param
  *\param bool Realys[8]
  *\param unsigned int Time
  *\brief Function will send according to parameter corresponding command. Function is overloaded.
  * Function is using method choose, which makes from boolen array byte. Parameter Time is 2 byte integer, which is decompposed into two separated bytes.
  */
void K8090::sendCommand(unsigned char used_relays_, int param, unsigned int Time)
{
    switch (param)
     {
     case 41: sendStartRelayTimer(used_relays_, Time); break;
     case 42: sendSetRelayTimer(used_relays_, Time); break;
     default: qDebug() << "Wrong choice";
     }
}
void K8090::sendCommand(unsigned char used_relays_, int param, bool option, bool notused)  // NO LINTAGE
// Query timer delay. 1 will be used for
{
    if (param == 44)  // Total delay Time and 2 for remaining
     {                // delay time
     unsigned char choise;
     if (option)
     {
      choise =  1 << 0;
     }else{
          choise =  1 << 1;
     }
    sendQueryTimerDelay(used_relays_, choise);
    }
}
/*!
  *\fn K8090::sendCommand(bool Relays[8], int param, unsigned int Time)
  *\param int param
  *\param bool Realys[8]
  *\param bool option
  *\param bool notused
  *\brief Function will send according to parameter corresponding command. Function is overloaded.
  * Function is using method choose, which makes from boolen array byte. Parameter option represent
  * option between total delay time (TRUE) and remaining delay time (FALSE)
  */
void K8090::sendCommand(unsigned char used_relays_1_, unsigned char used_relays_2_, unsigned char used_relays_3_, int param, bool notused)  // NO LINTAGE
{
    if (param == 21)                                                           // Total delay Time and 2 for remaining
     {
      sendsetButtonMode(used_relays_1_, used_relays_2_, used_relays_3_);
     }else{
        qDebug() << "Wrong choise.";
     }
}
/*!
  *\fn K8090::sendCommand(bool Relays1[8], bool Relays2[8], bool Relays3[8], int param, bool notused)
  *\param int param
  *\param bool Realys1[8]
  *\param bool Realys2[8]
  *\param bool Realys3[8]
  *\param bool notused
  *\brief Function will send according to parameter corresponding command. Function is overloaded.
  * Function is using method choose, which makes from boolen array byte. Relays1 correspond to relays, which will be set on momentary mode, Relays2 correspond to relays, which will be set on toggle mode, Relays3 correspond to relays, which will be set on timed mode,
  */

/*!
 * \fn K8090::sendSwitchRelayOnCommand
 * \param chosen
 * \brief Switch on corresponding Relay or Relays
 */
void K8090::sendSwitchRelayOnCommand(unsigned char chosen)
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::SwitchRelayOn)][ii];
    cmd[2] = chosen;
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::SwitchRelayOn;
    qDebug() << byteToHex(cmd, n);
    onSendToSerial(cmd, n);
    completedTaskControl();
}

/*!
 * \fn K8090::sendSwitchRelayOffCommand
 * \param chosen
 * \brief Switch off corresponding Relay or Relays
 */
void K8090::sendSwitchRelayOffCommand(unsigned char chosen)
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::SwitchRelayOff)][ii];
    cmd[2] = chosen;
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::SwitchRelayOff;
    qDebug() << byteToHex(cmd, n);
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
 * \fn K8090::sendtoggleRelayCommand
 * \param chosen
 * \brief Toggle corresponding Relay or Relays.
 */
void K8090::sendtoggleRelayCommand(unsigned char chosen)
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::ToggleRelay)][ii];
    cmd[2] = chosen;
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::ToggleRelay;
    qDebug() << byteToHex(cmd, n) << " Relay toggled";
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
 * \fn sendsetButtonMode(unsigned char choose1, unsigned char choose2, unsigned char choose3)
 * \param unsigned char chose1
 * \param unsigned char chose2
 * \param unsigned char chose3
 * \brief Set Momentary, Toggle and Timed mode to corresponding relays.
 */
void K8090::sendsetButtonMode(unsigned char choose1, unsigned char choose2, unsigned char choose3)
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::SetButtonMode)][ii];
    cmd[2] = choose1;
    cmd[3] = choose2;
    cmd[4] = choose3;
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::SetButtonMode;
    qDebug() << byteToHex(cmd, n) << " Set button mode";
    onSendToSerial(cmd, n);
    completedTaskControl();
}

void K8090::sendStartRelayTimer(unsigned char chosen, unsigned int Time)  // Didn't tested for more than few seconds.
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::StartRelayTimer)][ii];
    cmd[2] = chosen;
    cmd[3] = highByt(Time);
    cmd[4] = lowByt(Time);
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::StartRelayTimer;
    qDebug() << byteToHex(cmd, n) << " Timer started " << Time;
    onSendToSerial(cmd, n);
    completedTaskControl(Time);
}
/*!
  *\fn K8090::sendStartRelayTimer(unsigned char chosen, unsigned int Time)
  *\param Time
  *\brief Start Timer for corresponding time and relays.
  */
void K8090::sendSetRelayTimer(unsigned char chosen, unsigned int Time)  // don't know, if it works.
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::SetRelayTimerDelay)][ii];
    cmd[2] = chosen;
    cmd[3] = highByt(Time);
    cmd[4] = lowByt(Time);
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::SetRelayTimerDelay;
    qDebug() << byteToHex(cmd, n) << " Timer set on " << Time;
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
  *\fn K8090::sendSetRelayTimer(unsigned char chosen, unsigned int Time)
  *\param Time
  *\brief Set Timer for corresponding time and relays.
  */
void K8090::sendQueryRelayStatus()  // don't know, if it works.
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::QueryRelayStatus)][ii];
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::QueryRelayStatus;
    qDebug() << byteToHex(cmd, n) << " request for relay status";
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
  *\fn K8090::sendQueryRelayStatus()
  *\brief Query the current status of all relays (on/off) and their timers (active/inactive).
  */
void K8090::sendQueryTimerDelay(unsigned char choose, unsigned char option)
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::QueryTimerDelay)][ii];
    cmd[2] = choose;
    cmd[3] = option;
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::QueryTimerDelay;
    qDebug() << byteToHex(cmd, n) << " request for first relay timer status";
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
  *\fn K8090::sendQueryTimer Delay()
  *\brief Query the current timer delay for first relay.
  */
void K8090::sendQueryButtonMode()
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
    cmd[ii] = bCommands[static_cast<int>(Command::QueryButtonMode)][ii];
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::QueryButtonMode;
    qDebug() << byteToHex(cmd, n) << "zažiadali sme o status časovača 1";
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
  *\fn K8090::sendQueryButtonMode()
  *\brief Query the current mode of each button.
  */
void K8090::sendRessetfactorydefaults()
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::ResetFactoryDefaults)][ii];
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::ResetFactoryDefaults;
    qDebug() << byteToHex(cmd, n) << "Zresetované";
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
  *\fn K8090::sendRessetfactorydefaults()
  *\brief Reset the board to factory defaults. All buttons are set to toggle mode and all timer delays are set to 5 seconds.
  */
void K8090::sendJumperStatus()
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::JumperStatus)][ii];
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::JumperStatus;
    qDebug() << byteToHex(cmd, n) << "niečo robí";
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
  *\fn K8090::sendJumperStatus()
  *\brief QChecks the position of the 'Event' jumper. If the jumper is set, the buttons no longer interact with the
relays but button events are still sent to the computer.
  */
void K8090::sendFirmwareVersion()
{
    int n = 7;  // Number of command bytes.
    unsigned char * cmd = new unsigned char[n];
    int ii;
    for (ii = 0; ii < 2; ++ii)
        cmd[ii] = bCommands[static_cast<int>(Command::FirmwareVersion)][ii];
    cmd[5] = checkSum(cmd, 5);
    cmd[6] = bEtxByte;
    lastCommand = Command::FirmwareVersion;
    qDebug() << byteToHex(cmd, n) << "v hexadecimálnej sústave vypíše parametre dosky";
    onSendToSerial(cmd, n);
    completedTaskControl();
}
/*!
  *\fn K8090::sendFirmwareVersion()
  *\brief Queries the firmware version of the board. The version number consists of the year and week
combination of the date the firmware was compiled.
  */
void K8090::completedTaskControl()
{
    bool result;
    result = serialPort_->waitForReadyRead(300);
    if (result)
    {completedTaskControl();
     qDebug() << "Still in progress, I'm waiting for end of command.";
    }else{
     qDebug() << "Done. I'm ready for other commands";
     return;
    }
}
/*!
  *\fn K8090::completedTaskControl()
  *\brief Controll if all commands is executed and Relay card don't response anymore.
  */
void K8090::completedTaskControl(int Time)  // Version for Timers
{bool result;
    result = serialPort_->waitForReadyRead(300+Time*1000);
    if (result)
    {completedTaskControl();
     qDebug() << "Still in progress, I'm waiting for end of command..";
    }else{
     qDebug() << "Done. I'm ready for other commands";
     return;
    }
}
/*!
  *\fn K8090::completedTaskControl(int Time)
  *\param int Time
  *\brief Controll if all commands is executed and Relay card don't response anymore.
  */
unsigned char K8090::lowByt(unsigned int number)
{
    unsigned char bytarr[2];
    bytarr[0] = (number)&(0xFF);
    bytarr[1] = (number>>8)&(0xFF);
    return bytarr[0];
}
/*!
  *\fn K8090::lowByt(number)
  *\brief Save first 8 bits of 16 bit integer.
  */
unsigned char K8090::highByt(unsigned int number)
{
    unsigned char bytarr[2];
    bytarr[0] = number&0xFF;
    bytarr[1] = (number>>8)&0xFF;
    return bytarr[1];
}
/*!
  *\fn K8090::highByt(unsigned int number)
  *\brief Save second 8 bits of 16 bit integer.
  */


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
bool K8090::validateResponse(const QString &msg, Command cmd)
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
bool K8090::validateResponse(const unsigned char *bMsg, int n, Command cmd)
{
    if (n >= 6) {
        for (int ii = 0; ii < 2; ++ii)
            if (bMsg[ii] != bCommands[static_cast<int>(cmd)][ii])
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
   \brief Fills arrays containing commands according to their importance.
 */
void K8090::fillCommandsArrays()
{
    strCommands[static_cast<int>(Command::SwitchRelayOn)] = switchRelayOnCmd;
    strCommands[static_cast<int>(Command::SwitchRelayOff)] = switchRelayOffCmd;
    strCommands[static_cast<int>(Command::ToggleRelay)] = toggleRelayCmd;
    strCommands[static_cast<int>(Command::QueryRelayStatus)] = queryRelayStatusCmd;
    strCommands[static_cast<int>(Command::SetButtonMode)] = setButtonModeCmd;
    strCommands[static_cast<int>(Command::QueryButtonMode)] = queryButtonModeCmd;
    strCommands[static_cast<int>(Command::StartRelayTimer)] = startRelayTimerCmd;
    strCommands[static_cast<int>(Command::SetRelayTimerDelay)] = setRelayTimerDelayCmd;
    strCommands[static_cast<int>(Command::QueryTimerDelay)] = queryTimerDelayCmd;
    strCommands[static_cast<int>(Command::ButtonStatus)] = buttonStatusCmd;
    strCommands[static_cast<int>(Command::RelayStatus)] = relayStatusCmd;
    strCommands[static_cast<int>(Command::ResetFactoryDefaults)] = resetFactoryDefaultsCmd;
    strCommands[static_cast<int>(Command::JumperStatus)] = jumperStatusCmd;
    strCommands[static_cast<int>(Command::FirmwareVersion)] = firmwareVersionCmd;

    bool ok;
    for (int ii = 0; ii < static_cast<int>(Command::None); ++ii) {
        bCommands[ii][0] = stxByte.toUInt(&ok, 16);
        bCommands[ii][1] = static_cast<unsigned char>(strCommands[ii].toUInt(&ok, 16));
    }

    bEtxByte = etxByte.toUInt(&ok, 16);
}

/*!
   \brief K8090::~K8090
 */
K8090::~K8090()
{
    serialPort_->close();
    delete serialPort_;
}
