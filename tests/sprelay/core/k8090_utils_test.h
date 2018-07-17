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
 * \file      k8090_utils_test.h
 * \brief     Tests for utility functions and data structures for K8090 class implementation.

 * \author    Jakub Klener <lumiksro@centrum.cz>
 * \date      2018-07-17
 * \copyright Copyright (C) 2018 Jakub Klener. All rights reserved.
 *
 * \copyright This project is released under the 3-Clause BSD License. You should have received a copy of the 3-Clause
 *            BSD License along with this program. If not, see https://opensource.org/licenses/.
 */


#ifndef SPRELAY_CORE_K8090_UTILS_TEST_H_
#define SPRELAY_CORE_K8090_UTILS_TEST_H_

#include <QObject>

#include "sprelay/test_suite/test_suite.h"

namespace sprelay {
namespace core {
namespace k8090 {
namespace impl_ {

class K8090UtilsTest : public QObject
{
    Q_OBJECT

private slots:  // NOLINT(whitespace/indent)
    void checkSum();
};

ADD_TEST(K8090UtilsTest)


class CommandTest : public QObject
{
    Q_OBJECT

private slots:  // NOLINT(whitespace/indent)
    void orEqual_data();
    void orEqual();
    void equality_data();
    void equality();
    void equalityFalse_data();
    void equalityFalse();
    void isCompatible_data();
    void isCompatible();
    void isNotCompatible_data();
    void isNotCompatible();
};

ADD_TEST(CommandTest)


class CardMessageTest : public QObject
{
    Q_OBJECT

private slots:  // NOLINT(whitespace/indent)
    void constructors();
    void checksumMessage();
    void isValid();
    void commandByte();
};

ADD_TEST(CardMessageTest)

}  // namespace impl_
}  // namespace k8090
}  // namespace core
}  // namespace sprelay

#endif  // SPRELAY_CORE_K8090_UTILS_TEST_H_
