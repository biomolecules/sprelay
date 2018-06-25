/***************************************************************************
**                                                                        **
**  Controlling interface for K8090 8-Channel Relay Card from Velleman    **
**  through usb using virtual serial port in Qt.                          **
**  Copyright (C) 2018 Jakub Klener                                       **
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

#ifndef SPRELAY_CORE_MOCK_SERIAL_PORT_TEST_H_
#define SPRELAY_CORE_MOCK_SERIAL_PORT_TEST_H_

#include <QObject>
#include <QString>

#include <memory>

#include "mock_serial_port.h"


namespace sprelay {
namespace core {

class MockSerialPort;

class MockSerialPortTest : public QObject
{
    Q_OBJECT
public:  // NOLINT(whitespace/indent)
    static const int kCommandTimeoutMs;
    static const int kDelayBetweenCommandsMs;

private slots:  // NOLINT(whitespace/indent)
    void init();
    void cleanup();
    void commandBenchmark_data();
    void commandBenchmark();
    void jumperStatus();
    void firmwareVersion();
    void queryAllTimers();
    void setMoreTimers();
    void startTimer();
    void defaultTimer();
    void moreTimers();
    void moreDefaultTimers();
    // TODO(lumik): add test for factory defaults command

private:  // NOLINT(whitespace/indent)
    unsigned char checkSum(const unsigned char *bMsg, int n);

    bool compareResponse(const unsigned char *response, const unsigned char *expected);
    void sendCommand(MockSerialPort *serial_port, const unsigned char *command) const;
    bool measureCommandWithResponse(MockSerialPort *serial_port, const unsigned char *message, qint64 *elapsed_ms);

    std::unique_ptr<MockSerialPort> mock_serial_port_;
};

}  // namespace core
}  // namespace sprelay

#endif  // SPRELAY_CORE_MOCK_SERIAL_PORT_TEST_H_
