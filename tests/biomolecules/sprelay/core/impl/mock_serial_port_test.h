// -*-c++-*-

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

/*!
 * \file      mock_serial_port_test.h
 * \brief     The biomolecules::sprelay::core::MockSerialPortTest class which implements tests for
 *            biomolecules::sprelay::core::MockSerialPort.
 *
 * \author    Jakub Klener <lumiksro@centrum.cz>
 * \date      2018-04-24
 * \copyright Copyright (C) 2018 Jakub Klener. All rights reserved.
 *
 * \copyright This project is released under the 3-Clause BSD License. You should have received a copy of the 3-Clause
 *            BSD License along with this program. If not, see https://opensource.org/licenses/.
 */


#ifndef BIOMOLECULES_SPRELAY_CORE_IMPL_MOCK_SERIAL_PORT_TEST_H_
#define BIOMOLECULES_SPRELAY_CORE_IMPL_MOCK_SERIAL_PORT_TEST_H_

#include <QObject>
#include <QString>

#include <memory>

#include "lumik/qtest_suite/qtest_suite.h"

#include "biomolecules/sprelay/core/mock_serial_port.h"

namespace biomolecules {
namespace sprelay {
namespace core {

class MockSerialPort;

class MockSerialPortTest : public QObject
{
    Q_OBJECT
public:
    static const int kCommandTimeoutMs;
    static const int kDelayBetweenCommandsMs;

private slots:
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

private:
    bool compareResponse(const unsigned char* response, const unsigned char* expected);
    void sendCommand(MockSerialPort* serial_port, const unsigned char* command) const;
    bool measureCommandWithResponse(MockSerialPort* serial_port, const unsigned char* message, qint64* elapsed_ms);

    std::unique_ptr<MockSerialPort> mock_serial_port_;
};

// NOLINTNEXTLINE(cert-err58-cpp, fuchsia-statically-constructed-objects)
ADD_TEST(MockSerialPortTest)

}  // namespace core
}  // namespace sprelay
}  // namespace biomolecules

#endif  // BIOMOLECULES_SPRELAY_CORE_IMPL_MOCK_SERIAL_PORT_TEST_H_
