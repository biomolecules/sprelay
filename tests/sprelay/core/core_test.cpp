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

#include <QCoreApplication>
#include <QtTest>

#include <memory>

#include "command_queue_test.h"
#include "k8090_test.h"
#include "serial_port_utils_test.h"

using namespace sprelay::core;  // NOLINT(build/namespaces)

int main(int argc, char **argv)
{
    int status = 0;

    // SerialPortUtilsTest
    std::unique_ptr<SerialPortUtilsTest> serial_port_utils_test{new SerialPortUtilsTest};
    status |= QTest::qExec(serial_port_utils_test.get(), argc, argv);

    // CommandQueueTest
    std::unique_ptr<command_queue::CommandQueueTest> command_queue_test{new command_queue::CommandQueueTest};
    status |= QTest::qExec(command_queue_test.get(), argc, argv);

    // K8090Test
    std::unique_ptr<K8090Test> k8090_test{new K8090Test};
    status |= QTest::qExec(k8090_test.get(), argc, argv);

    return status;
}
