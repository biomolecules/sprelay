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

#include "unified_serial_port_test.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QStringBuilder>
#include <QTimer>
#include <QtTest>

#include <algorithm>
#include <chrono>
#include <list>
#include <thread>
#include <utility>

#include "k8090.h"
#include "unified_serial_port.h"

namespace sprelay {
namespace core {

const int UnifiedSerialPortTest::kCommandTimeoutMs = 50;


void UnifiedSerialPortTest::initTestCase()
{
    real_card_present_ = false;
    foreach (const ComPortParams &params, UnifiedSerialPort::availablePorts()) {  // NOLINT(whitespace/parens)
        if (params.product_identifier == K8090::kProductID && params.vendor_identifier == K8090::kVendorID) {
            if (params.port_name != UnifiedSerialPort::kMockPortName) {
                real_card_port_name_ = params.port_name;
                real_card_present_ = true;
                break;
            }
        }
    }
}


void UnifiedSerialPortTest::availablePortsTest()
{
    // test, if the mock port is always present
    bool mock_found = false;
    foreach (const ComPortParams &params, UnifiedSerialPort::availablePorts()) {  // NOLINT(whitespace/parens)
        if (params.port_name == UnifiedSerialPort::kMockPortName
                && params.product_identifier == K8090::kProductID
                && params.vendor_identifier == K8090::kVendorID) {
            mock_found = true;
        }
    }
    QCOMPARE(mock_found, true);
}


void UnifiedSerialPortTest::realBenchmark_data()
{
    if (!real_card_present_) {
        QSKIP("Benchmark of real serial port needs a real K8090 card connected.");
    }
    QTest::addColumn<const unsigned char *>("prepare");
    QTest::addColumn<int>("n_prepare");
    QTest::addColumn<const unsigned char *>("message");
    QTest::addColumn<const unsigned char *>("response");

    // switch relay on
    //                                        STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare1   = nullptr;
    static const unsigned char on1[]       = {0x04, 0x11, 0x01, 0x00, 0x00, 0xea, 0x0f};
    static const unsigned char response1[] = {0x04, 0x51, 0x00, 0x01, 0x00, 0xaa, 0x0f};
    QTest::newRow("Switch on") << prepare1 << 0 << &on1[0] << &response1[0];

    // switch relay all on
    //                                        STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare2   = nullptr;
    static const unsigned char on2[]       = {0x04, 0x11, 0xff, 0x00, 0x00, 0xec, 0x0f};
    static const unsigned char response2[] = {0x04, 0x51, 0x00, 0xff, 0x00, 0xac, 0x0f};
    QTest::newRow("Switch all on") << prepare2 << 0 << &on2[0] << &response2[0];

    // switch relay off
    static const unsigned char prepare3[]  = {
    //  STX   CMD   MASK  PAR1  PAR2  CHK   ETX
        0x04, 0x11, 0x02, 0x00, 0x00, 0xe9, 0x0f  // switch relay on
    };
    //                                   STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char off3[] = {0x04, 0x12, 0x02, 0x00, 0x00, 0xe8, 0x0f};
    static const unsigned char response3[] = {0x04, 0x51, 0x02, 0x00, 0x00, 0xa9, 0x0f};
    QTest::newRow("Switch off") << &prepare3[0] << 1 << &off3[0] << &response3[0];

    // toggle relay on
    //                                        STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare4   = nullptr;
    static const unsigned char toggle4[]   = {0x04, 0x14, 0x04, 0x00, 0x00, 0xe4, 0x0f};
    static const unsigned char response4[] = {0x04, 0x51, 0x00, 0x04, 0x00, 0xa7, 0x0f};
    QTest::newRow("toggle on") << prepare4 << 0 << &toggle4[0] << &response4[0];

    // toggle relay off
    static const unsigned char prepare5[]  = {
    //  STX   CMD   MASK  PAR1  PAR2  CHK   ETX
        0x04, 0x11, 0x08, 0x00, 0x00, 0xe3, 0x0f  // switch relay on
    };
    //                                        STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char toggle5[]   = {0x04, 0x14, 0x08, 0x00, 0x00, 0xe0, 0x0f};
    static const unsigned char response5[] = {0x04, 0x51, 0x08, 0x00, 0x00, 0xa3, 0x0f};
    QTest::newRow("toggle off") << &prepare5[0] << 1 << &toggle5[0] << &response5[0];

    // set button mode
    static const unsigned char prepare6[]  = {
    //  STX   CMD   MASK  PAR1  PAR2  CHK   ETX
        0x04, 0x21, 0x10, 0xcf, 0x20, 0xdc, 0x0f  // set relay 5 to momentary and 6 to timed and all the rest to toggle
    };
    //                                          STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char query_mode6[] = {0x04, 0x22, 0x00, 0x00, 0x00, 0xda, 0x0f};
    static const unsigned char response6[]   = {0x04, 0x22, 0x10, 0xcf, 0x20, 0xdb, 0x0f};
    QTest::newRow("button mode") << &prepare6[0] << 1 << &query_mode6[0] << &response6[0];

    // set relay timer delay
    static const unsigned char prepare7[]  = {
    //  STX   CMD   MASK  PAR1  PAR2  CHK   ETX
        0x04, 0x42, 0x40, 0x00, 0x01, 0x79, 0x0f  // set relay 7 timer to one second
    };
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char query_timer7[] = {0x04, 0x44, 0x40, 0x00, 0x00, 0x78, 0x0f};
    // !!!BEWARE: there is mistake in the manual, if first byte of par1 is 0, a total timer delay is queried, if 1, a
    // remaining timer delay is queried
    static const unsigned char response7[]    = {0x04, 0x44, 0x40, 0x00, 0x01, 0x77, 0x0f};
    QTest::newRow("query timer delay") << &prepare7[0] << 1 << &query_timer7[0] << &response7[0];

    // query relay status
    //                                          STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare8       = nullptr;
    static const unsigned char query_status8[] = {0x04, 0x18, 0x00, 0x00, 0x00, 0xe4, 0x0f};
    static const unsigned char response8[]     = {0x04, 0x51, 0x00, 0x00, 0x00, 0xab, 0x0f};
    QTest::newRow("button status") << prepare8 << 0 << &query_status8[0] << &response8[0];

    // test right values of factory defaults //
    //***************************************//

    // query button modes
    //                                          STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare9     = nullptr;
    static const unsigned char query_mode9[] = {0x04, 0x22, 0x00, 0x00, 0x00, 0xda, 0x0f};
    static const unsigned char response9[]   = {0x04, 0x22, 0x00, 0xff, 0x00, 0xdb, 0x0f};
    QTest::newRow("factory defaults - button modes") << prepare9 << 0 << &query_mode9[0] << &response9[0];

    // query timer delays
    // timer 1
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare10      = nullptr;
    static const unsigned char query_timer10[] = {0x04, 0x44, 0x01, 0x00, 0x00, 0xb7, 0x0f};
    static const unsigned char response10[]    = {0x04, 0x44, 0x01, 0x00, 0x05, 0xb2, 0x0f};
    QTest::newRow("factory defaults - query timer 1") << prepare10 << 0 << &query_timer10[0] << &response10[0];

    // timer 2
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare11      = nullptr;
    static const unsigned char query_timer11[] = {0x04, 0x44, 0x02, 0x00, 0x00, 0xb6, 0x0f};
    static const unsigned char response11[]    = {0x04, 0x44, 0x02, 0x00, 0x05, 0xb1, 0x0f};
    QTest::newRow("factory defaults - query timer 2") << prepare11 << 0 << &query_timer11[0] << &response11[0];

    // timer 3
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare12      = nullptr;
    static const unsigned char query_timer12[] = {0x04, 0x44, 0x04, 0x00, 0x00, 0xb4, 0x0f};
    static const unsigned char response12[]    = {0x04, 0x44, 0x04, 0x00, 0x05, 0xaf, 0x0f};
    QTest::newRow("factory defaults - query timer 3") << prepare12 << 0 << &query_timer12[0] << &response12[0];

    // timer 4
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare13      = nullptr;
    static const unsigned char query_timer13[] = {0x04, 0x44, 0x08, 0x00, 0x00, 0xb0, 0x0f};
    static const unsigned char response13[]    = {0x04, 0x44, 0x08, 0x00, 0x05, 0xab, 0x0f};
    QTest::newRow("factory defaults - query timer 4") << prepare13 << 0 << &query_timer13[0] << &response13[0];

    // timer 5
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare14      = nullptr;
    static const unsigned char query_timer14[] = {0x04, 0x44, 0x10, 0x00, 0x00, 0xa8, 0x0f};
    static const unsigned char response14[]    = {0x04, 0x44, 0x10, 0x00, 0x05, 0xa3, 0x0f};
    QTest::newRow("factory defaults - query timer 5") << prepare14 << 0 << &query_timer14[0] << &response14[0];

    // timer 6
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare15      = nullptr;
    static const unsigned char query_timer15[] = {0x04, 0x44, 0x20, 0x00, 0x00, 0x98, 0x0f};
    static const unsigned char response15[]    = {0x04, 0x44, 0x20, 0x00, 0x05, 0x93, 0x0f};
    QTest::newRow("factory defaults - query timer 6") << prepare15 << 0 << &query_timer15[0] << &response15[0];

    // timer 7
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare16      = nullptr;
    static const unsigned char query_timer16[] = {0x04, 0x44, 0x40, 0x00, 0x00, 0x78, 0x0f};
    static const unsigned char response16[]    = {0x04, 0x44, 0x40, 0x00, 0x05, 0x73, 0x0f};
    QTest::newRow("factory defaults - query timer 7") << prepare16 << 0 << &query_timer16[0] << &response16[0];

    // timer 8
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char *prepare17      = nullptr;
    static const unsigned char query_timer17[] = {0x04, 0x44, 0x80, 0x00, 0x00, 0x38, 0x0f};
    static const unsigned char response17[]    = {0x04, 0x44, 0x80, 0x00, 0x05, 0x33, 0x0f};
    QTest::newRow("factory defaults - query timer 8") << prepare17 << 0 << &query_timer17[0] << &response17[0];
}


void UnifiedSerialPortTest::realBenchmark()
{
    std::unique_ptr<UnifiedSerialPort> serial_port = createSerialPort(real_card_port_name_);
    if (!serial_port) {
        QFAIL(qPrintable(QString{"Port '%1' can't be opened."}.arg(real_card_port_name_)));
    }

    // fetch data
    QFETCH(const unsigned char *, prepare);
    QFETCH(int, n_prepare);
    QFETCH(const unsigned char *, message);
    QFETCH(const unsigned char *, response);

    // prepare relay for the command
    for (int i = 0; i < n_prepare; ++i) {
        sendCommand(serial_port.get(), &prepare[i * 7]);
    }
    serial_port->readAll();

    // benchmark the command
    qint64 elapsed_time;
    if (!measureCommandWithResponse(serial_port.get(), message, &elapsed_time)) {
        QFAIL("There is no response from the card.");
    }
    QTest::setBenchmarkResult(elapsed_time, QTest::WalltimeMilliseconds);

    // check for expected response
    QByteArray data = serial_port->readAll();
    int n = data.size();
    const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
    if (n != 7) {
        QFAIL(qPrintable(QString{"Response has %1 but should have 7"}.arg(n)));
    }
    QVERIFY2(compareResponse(buffer, response),
             qPrintable(QString{"The response '%1' does not match the expected %2."}
                        .arg(byte_to_hex(buffer, 7)).arg(byte_to_hex(response, 7))));
}

void UnifiedSerialPortTest::realJumperStatus()
{
    if (!real_card_present_) {
        QSKIP("Benchmark of real serial port needs real K8090 card connected.");
    }
    std::unique_ptr<UnifiedSerialPort> serial_port = createSerialPort(real_card_port_name_);
    if (!serial_port) {
        QFAIL(qPrintable(QString{"Port '%1' can't be opened."}.arg(real_card_port_name_)));
    }

    // benchmark the command
    //                                           STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char query_jumper[] = {0x04, 0x70, 0x00, 0x00, 0x00, 0x8c, 0x0f};
    static const unsigned char jumper_off[]   = {0x04, 0x70, 0x00, 0x00, 0x00, 0x8c, 0x0f};
    qint64 elapsed_time;
    if (!measureCommandWithResponse(serial_port.get(), query_jumper, &elapsed_time)) {
        QFAIL("There is no response from the card.");
    }
    QTest::setBenchmarkResult(elapsed_time, QTest::WalltimeMilliseconds);

    // check for expected response
    QByteArray data = serial_port->readAll();
    int n = data.size();
    const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
    if (n != 7) {
        QFAIL(qPrintable(QString{"Response has %1 but should have 7"}.arg(n)));
    }
    QVERIFY2(buffer[1] == jumper_off[1],
             qPrintable(QString{"The response '%1' does not match the expected %2."}
                        .arg(byte_to_hex(&buffer[1], 1)).arg(byte_to_hex(&jumper_off[1], 1))));
    if (buffer[3]) {
        qDebug() << "Jumper is switched on.";
    } else {
        qDebug() << "Jumper is switched off.";
    }
}

void UnifiedSerialPortTest::realFirmwareVersion()
{
    if (!real_card_present_) {
        QSKIP("This test needs a real K8090 card connected.");
    }
    std::unique_ptr<UnifiedSerialPort> serial_port = createSerialPort(real_card_port_name_);
    if (!serial_port) {
        QFAIL(qPrintable(QString{"Port '%1' can't be opened."}.arg(real_card_port_name_)));
    }

    // benchmark the command
    //                                            STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char query_version[] = {0x04, 0x71, 0x00, 0x00, 0x00, 0x8b, 0x0f};
    static const unsigned char version[]    = {0x04, 0x71, 0x00, 0x00, 0x00, 0x8b, 0x0f};
    qint64 elapsed_time;
    if (!measureCommandWithResponse(serial_port.get(), query_version, &elapsed_time)) {
        QFAIL("There is no response from the card.");
    }
    QTest::setBenchmarkResult(elapsed_time, QTest::WalltimeMilliseconds);

    // check for expected response
    QByteArray data = serial_port->readAll();
    int n = data.size();
    const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
    if (n != 7) {
        QFAIL(qPrintable(QString{"Response has %1 but should have 7 bytes"}.arg(n)));
    }
    QVERIFY2(buffer[1] == version[1],
             qPrintable(QString{"The response '%1' does not match the expected %2."}
                        .arg(byte_to_hex(&buffer[1], 1)).arg(byte_to_hex(&version[1], 1))));
    qDebug() << QString{"Firmware version is: year = %1, week = %2."}.arg(2000 + static_cast<int>(buffer[3]))
            .arg(static_cast<int>(buffer[4]));
}

void UnifiedSerialPortTest::realQueryAllTimers()
{
    if (!real_card_present_) {
        QSKIP("This test needs a real K8090 card connected.");
    }
    std::unique_ptr<UnifiedSerialPort> serial_port = createSerialPort(real_card_port_name_);
    if (!serial_port) {
        QFAIL(qPrintable(QString{"Port '%1' can't be opened."}.arg(real_card_port_name_)));
    }

    // benchmark the command
    //                                            STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char query_timers[] = {0x04, 0x44, 0xff, 0x00, 0x00, 0xb9, 0x0f};
    static const unsigned char responses[] = {
    //  STX   CMD   MASK  PAR1  PAR2  CHK   ETX
        0x04, 0x44, 0x01, 0x00, 0x05, 0xb2, 0x0f,
        0x04, 0x44, 0x02, 0x00, 0x05, 0xb1, 0x0f,
        0x04, 0x44, 0x04, 0x00, 0x05, 0xaf, 0x0f,
        0x04, 0x44, 0x08, 0x00, 0x05, 0xab, 0x0f,
        0x04, 0x44, 0x10, 0x00, 0x05, 0xa3, 0x0f,
        0x04, 0x44, 0x20, 0x00, 0x05, 0x93, 0x0f,
        0x04, 0x44, 0x40, 0x00, 0x05, 0x73, 0x0f,
        0x04, 0x44, 0x80, 0x00, 0x05, 0x33, 0x0f
    };

    // measure time to get all responses and compute number of chunks
    qint64 elapsed_ms;
    QTimer timer;
    timer.setSingleShot(true);
    QElapsedTimer elapsed_timer;

    std::list<int> remaining_responses{0, 7, 14, 21, 28, 35, 42, 49};
    std::list<int> chunk_list;

    elapsed_timer.start();
    serial_port->write(reinterpret_cast<const char*>(query_timers), 7);
    serial_port->flush();

    timer.start(kCommandTimeoutMs * 8);
    while (!remaining_responses.empty() && timer.isActive()) {
        QEventLoop loop;
        connect(serial_port.get(), &UnifiedSerialPort::readyRead, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        loop.exec();

        // check for expected response
        QByteArray data = serial_port->readAll();
        int n = data.size();
        if (n % 7 != 0) {
            QFAIL(qPrintable(QString{"Response has %1 bytes which is not a multiply of 7."}.arg(n)));
        }
        const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
        std::list<int>::iterator it;
        for (int i = 0; i < n; i += 7) {
            it = std::find_if(remaining_responses.begin(), remaining_responses.end(),
                    [&buffer, &i, this](int j) { return compareResponse(&buffer[i], &responses[j]); });
            QVERIFY2(it != remaining_responses.end(),
                     qPrintable(QString{"The response '%1' does not match."}.arg(byte_to_hex(&buffer[i], 7))));
            remaining_responses.erase(it);
        }
        chunk_list.emplace_back(n / 7);
    }
    elapsed_ms = elapsed_timer.elapsed();
    QVERIFY2(timer.isActive(), qPrintable(QString{"Only %1 out of %2 responses were obtained."}
                         .arg(8 - remaining_responses.size()).arg(8)));
    QTest::setBenchmarkResult(elapsed_ms, QTest::WalltimeMilliseconds);
    QString chunk_str = QString{"{%1"}.arg(*chunk_list.cbegin());
    for_each(std::next(chunk_list.cbegin()), chunk_list.cend(),
        [&chunk_list, &chunk_str](int i){ chunk_str.append(QString{", %1"}.arg(i)); });
    chunk_str.append("}");
    qDebug() << QString{"Responses were gathered in %1."}.arg(chunk_str);
}

void UnifiedSerialPortTest::realTestTimer()
{
    if (!real_card_present_) {
        QSKIP("This test needs a real K8090 card connected.");
    }
    std::unique_ptr<UnifiedSerialPort> serial_port = createSerialPort(real_card_port_name_);
    if (!serial_port) {
        QFAIL(qPrintable(QString{"Port '%1' can't be opened."}.arg(real_card_port_name_)));
    }

    // benchmark the command
    //                                          STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char start_timer[] = {0x04, 0x41, 0x10, 0x00, 0x03, 0xa8, 0x0f};
    static const unsigned char on_status[]   = {0x04, 0x51, 0x00, 0x10, 0x10, 0x8b, 0x0f};
    static const unsigned char query_timer[] = {0x04, 0x44, 0x10, 0x01, 0x00, 0xa7, 0x0f};
    static const unsigned char remaining_timer = 0x44;
    static const unsigned char off_status[]  = {0x04, 0x51, 0x10, 0x00, 0x00, 0x9b, 0x0f};

    // measure timer
    qint64 elapsed_ms;
    QElapsedTimer elapsed_timer;

    elapsed_timer.start();
    {  // start timer
        qint64 start_timer_elapsed_ms;
        if (!measureCommandWithResponse(serial_port.get(), start_timer, &start_timer_elapsed_ms)) {
            QFAIL("There is no response from the card.");
        }
        QTest::setBenchmarkResult(start_timer_elapsed_ms, QTest::WalltimeMilliseconds);

        // check for expected response
        QByteArray data = serial_port->readAll();
        int n = data.size();
        if (n != 7) {
            QFAIL(qPrintable(QString{"Response has %1 bytes which is not a multiply of 7."}.arg(n)));
        }
        const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
        QVERIFY2(compareResponse(buffer, on_status),
                 qPrintable(QString{"The response '%1' does not match the expected %2."}
                            .arg(byte_to_hex(buffer, 7)).arg(byte_to_hex(on_status, 7))));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    {  // query remaining time
        qint64 dummy_elapsed_ms;
        if (!measureCommandWithResponse(serial_port.get(), query_timer, &dummy_elapsed_ms)) {
            QFAIL("There is no response from the card.");
        }

        // check for expected response
        QByteArray data = serial_port->readAll();
        int n = data.size();
        if (n != 7) {
            QFAIL(qPrintable(QString{"Response has %1 bytes which is not a multiply of 7."}.arg(n)));
        }
        const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
        QVERIFY2(buffer[1] == remaining_timer,
                 qPrintable(QString{"The response '%1' does not match the expected %2."}
                            .arg(byte_to_hex(buffer, 1)).arg(byte_to_hex(&remaining_timer, 1))));
        qDebug() << QString{"Remaining timer is %1s"}.arg(256 * buffer[3] + buffer[4]);
    }
    {  // wait for relay timer to elapse
        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        connect(serial_port.get(), &UnifiedSerialPort::readyRead, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(3000);
        loop.exec();
        QVERIFY2(timer.isActive(), "There is no response from the card.");
        // check for expected response
        QByteArray data = serial_port->readAll();
        int n = data.size();
        if (n != 7) {
            QFAIL(qPrintable(QString{"Response has %1 bytes which is not a multiply of 7."}.arg(n)));
        }
        const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
        QVERIFY2(compareResponse(buffer, off_status),
                 qPrintable(QString{"The response '%1' does not match the expected %2."}
                            .arg(byte_to_hex(buffer, 7)).arg(byte_to_hex(off_status, 7))));
    }
    elapsed_ms = elapsed_timer.elapsed();
    qDebug() << QString{"The timer took %1ms"}.arg(elapsed_ms);
}

void UnifiedSerialPortTest::realTestDefaultTimer()
{
    if (!real_card_present_) {
        QSKIP("This test needs a real K8090 card connected.");
    }
    std::unique_ptr<UnifiedSerialPort> serial_port = createSerialPort(real_card_port_name_);
    if (!serial_port) {
        QFAIL(qPrintable(QString{"Port '%1' can't be opened."}.arg(real_card_port_name_)));
    }

    // benchmark the command
    //                                          STX   CMD   MASK  PAR1  PAR2  CHK   ETX
    static const unsigned char set_timer[]   = {0x04, 0x42, 0x20, 0x00, 0x01, 0x99, 0x0f};
    static const unsigned char start_timer[] = {0x04, 0x41, 0x20, 0x00, 0x00, 0x9b, 0x0f};
    static const unsigned char on_status[]   = {0x04, 0x51, 0x00, 0x20, 0x20, 0x6b, 0x0f};
    static const unsigned char off_status[]  = {0x04, 0x51, 0x20, 0x00, 0x00, 0x8b, 0x0f};

    sendCommand(serial_port.get(), set_timer);

    // measure timer
    qint64 elapsed_ms;
    QElapsedTimer elapsed_timer;

    elapsed_timer.start();
    {  // start timer
        qint64 start_timer_elapsed_ms;
        if (!measureCommandWithResponse(serial_port.get(), start_timer, &start_timer_elapsed_ms)) {
            QFAIL("There is no response from the card.");
        }
        QTest::setBenchmarkResult(start_timer_elapsed_ms, QTest::WalltimeMilliseconds);

        // check for expected response
        QByteArray data = serial_port->readAll();
        int n = data.size();
        if (n != 7) {
            QFAIL(qPrintable(QString{"Response has %1 bytes which is not a multiply of 7."}.arg(n)));
        }
        const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
        QVERIFY2(compareResponse(buffer, on_status),
                 qPrintable(QString{"The response '%1' does not match the expected %2."}
                            .arg(byte_to_hex(buffer, 7)).arg(byte_to_hex(on_status, 7))));
    }
    {  // wait for relay timer to elapse
        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        connect(serial_port.get(), &UnifiedSerialPort::readyRead, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(3000);
        loop.exec();
        QVERIFY2(timer.isActive(), "There is no response from the card.");
        // check for expected response
        QByteArray data = serial_port->readAll();
        int n = data.size();
        if (n != 7) {
            QFAIL(qPrintable(QString{"Response has %1 bytes which is not a multiply of 7."}.arg(n)));
        }
        const unsigned char *buffer = reinterpret_cast<const unsigned char*>(data.constData());
        QVERIFY2(compareResponse(buffer, off_status),
                 qPrintable(QString{"The response '%1' does not match the expected %2."}
                            .arg(byte_to_hex(buffer, 7)).arg(byte_to_hex(off_status, 7))));
    }
    elapsed_ms = elapsed_timer.elapsed();
    qDebug() << QString{"The timer took %1ms"}.arg(elapsed_ms);
}


std::unique_ptr<UnifiedSerialPort> UnifiedSerialPortTest::createSerialPort(QString port_name) const
{
    std::unique_ptr<UnifiedSerialPort> serial_port{new UnifiedSerialPort};
    serial_port->setPortName(port_name);
    serial_port->setBaudRate(QSerialPort::Baud19200);
    serial_port->setDataBits(QSerialPort::Data8);
    serial_port->setParity(QSerialPort::NoParity);
    serial_port->setStopBits(QSerialPort::OneStop);
    serial_port->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial_port->isOpen()) {
        if (!serial_port->open(QIODevice::ReadWrite)) {
            serial_port.reset(nullptr);
        }
    }
    resetRelays(serial_port.get());
    return std::move(serial_port);
}


void UnifiedSerialPortTest::resetRelays(UnifiedSerialPort *serial_port) const
{
    const unsigned char factory_defaults[] = {0x04, 0x66, 0x00, 0x00, 0x00, 0x96, 0x0f};
    sendCommand(serial_port, factory_defaults);
    const unsigned char switch_all_relays_off[] = {0x04, 0x12, 0xff, 0x00, 0x00, 0xEB, 0x0f};
    sendCommand(serial_port, switch_all_relays_off);
    serial_port->readAll();
}


bool UnifiedSerialPortTest::compareResponse(const unsigned char *response, const unsigned char *expected)
{
    unsigned char check_sum = checkSum(expected, 5);
    if (check_sum != expected[5]) {
        qDebug() << "Check sum should be:" << byte_to_hex(&check_sum, 1);
        return false;
    }
    for (int i = 0; i < 7; ++i) {
        if (response[i] != expected[i]) {
            return false;
        }
    }
    return true;
}


void UnifiedSerialPortTest::sendCommand(UnifiedSerialPort *serial_port, const unsigned char *command) const
{
    serial_port->write(reinterpret_cast<const char*>(command), 7);
    serial_port->flush();
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    std::this_thread::sleep_for(std::chrono::milliseconds(kCommandTimeoutMs));
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

bool UnifiedSerialPortTest::measureCommandWithResponse(UnifiedSerialPort *serial_port, const unsigned char *message,
                                                       qint64 *elapsed_ms)
{
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(serial_port, &UnifiedSerialPort::readyRead, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QElapsedTimer elapsed_timer;
    elapsed_timer.start();
    serial_port->write(reinterpret_cast<const char*>(message), 7);
    serial_port->flush();
    timer.start(kCommandTimeoutMs);
    loop.exec();
    *elapsed_ms = elapsed_timer.elapsed();
    if (!timer.isActive()) {
        return false;
    } else {
        return true;
    }
}


// helper method which computes checksum from binary command representation
unsigned char UnifiedSerialPortTest::checkSum(const unsigned char *bMsg, int n)
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

}  // namespace core
}  // namespace sprelay
