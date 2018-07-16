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
 * \file      k8090_utils.cpp
 * \brief     Utility functions and data structures for K8090 class implementation.

 * \author    Jakub Klener <lumiksro@centrum.cz>
 * \date      2018-07-16
 * \copyright Copyright (C) 2018 Jakub Klener. All rights reserved.
 *
 * \copyright This project is released under the 3-Clause BSD License. You should have received a copy of the 3-Clause
 *            BSD License along with this program. If not, see https://opensource.org/licenses/.
 */

#include "k8090_utils.h"

namespace sprelay {
namespace core {
namespace k8090 {

/*!
    \defgroup k8090_impl K8090 private
    \ingroup k8090
    \brief Private utilities of K8090 implementation.
*/

/*!
    \namespace sprelay::core::k8090::impl_
    \ingroup k8090_impl
    \brief Contains private utilities of K8090 implementation.
*/
namespace impl_ {


/*!
    \enum sprelay::core::impl_::TimerDelayType
    \ingroup k8090_impl
    \brief Scoped enumeration listing timer delay types.

    See the Velleman %K8090 card manual for more info about timer delay types.
*/
/*!
    \var sprelay::core::k8090::impl_::TimerDelayType::TOTAL
    \brief Total timer time.
*/
/*!
    \var sprelay::core::k8090::impl_::TimerDelayType::REMAINING
    \brief Currently remaining timer time.
*/
/*!
    \var sprelay::core::k8090::impl_::TimerDelayType::ALL
    \brief Determines the highest element.
*/


/*!
    \struct sprelay::core::k8090::impl_::Command
    \ingroup k8090_impl
    \brief Command representation.

    It is used for command comparisons and in command_queue::CommandQueue.
*/

/*!
    \typedef Command::IdType
    \brief Typename of id

    Required by command_queue::CommandQueue API.
*/

/*!
    \typedef Command::NumberType
    \brief Underlying typename of id

    Required by command_queue::CommandQueue API.
*/

/*!
    \fn Command::Command()
    \brief Default constructor.

    Initializes its id member to k8090::CommandID::NONE so the Command can be used as return value indicating failure.
*/

/*!
    \fn explicit Command::Command(IdType id, int priority = 0, unsigned char mask = 0, unsigned char param1 = 0,
    unsigned char param2 = 0)
    \brief Initializes Command.
*/

/*!
    \fn static NumberType Command::idAsNumber(IdType id)
    \brief Converts id to its underlying type.

    Required by command_queue::CommandQueue API.

    \param id Command id.
    \return Underlying type representation of the id.
*/

/*!
    \var Command::id
    \brief Command id.

    Required by command_queue::CommandQueue API.
*/

/*!
    \var Command::priority
    \brief Command priority.

    Required by command_queue::CommandQueue API.
*/

/*!
    \var Command::params
    \brief Stores command parameters.
*/


/*!
    \brief Merges the other Command.

    If the other command is k8090::CommandID::RELAY_ON and is merged to k8090::CommandID::RELAY_OFF or the
    oposite, the negation is merged. XOR is applied to k8090::CommandID::TOGGLE_RELAY and for
    k8090::CommandID::SET_BUTTON_MODE and duplicate assignments, the command is merged according to precedence
    stated in Velleman %K8090 card manual (momentary mode, toggle mode, timed mode from most important to less).

    The other commands are merged naturally as _or assignment_ operator to their members.

    \param other The other command.
    \return Merged command.
*/
Command & Command::operator|=(const Command &other) {
    switch (id) {
        // commands with special treatment
        case k8090::CommandID::RELAY_ON :
            if (other.id == k8090::CommandID::RELAY_OFF) {
                params[0] &= ~other.params[0];
            } else {
                params[0] |= other.params[0];
            }
            break;
        case k8090::CommandID::RELAY_OFF :
            if (other.id == k8090::CommandID::RELAY_ON) {
                params[0] &= ~other.params[0];
            } else {
                params[0] |= other.params[0];
            }
            break;
        case k8090::CommandID::TOGGLE_RELAY :
            params[0] ^= other.params[0];
            break;
        case k8090::CommandID::SET_BUTTON_MODE :
            params[0] |= other.params[0];
            params[1] |= other.params[1] & ~params[0];
            params[2] |= other.params[2] & ~params[1] & ~params[2];
            break;
        // commands with one relevant parameter mask
        case k8090::CommandID::START_TIMER :
        case k8090::CommandID::SET_TIMER :
        case k8090::CommandID::TIMER :
            params[0] |= other.params[0];
            break;
        // commands with no parameters
        //     case k8090::CommandID::QUERY_RELAY :
        //     case k8090::CommandID::BUTTON_MODE :
        //     case k8090::CommandID::RESET_FACTORY_DEFAULTS :
        //     case k8090::CommandID::JUMPER_STATUS :
        //     case k8090::CommandID::FIRMWARE_VERSION :
        default :
            break;
    }
    return *this;
}

/*!
    \fn bool Command::operator==(const Command &other) const
    \brief Compares two commands for equality.

    \param other The command to be compared.
    \return True if the commands are the same.
*/

/*!
    \fn bool Command::operator!=(const Command &other) const
    \brief Compares two commands for non-equality.

    \param other The command to be compared.
    \return True if the commands are different.
*/


/*!
    \brief Tests, if commands are compatible.

    Compatible commands can be merget by the Command::operator|=() operator.

    \param other The command to be stested.
    \return True if the commands are compatible.
*/
bool Command::isCompatible(const Command &other) const
{
    if (id != other.id) {
        switch (id) {
            case k8090::CommandID::RELAY_ON :
                if (other.id == k8090::CommandID::RELAY_OFF) {
                    return true;
                }
                return false;
            case k8090::CommandID::RELAY_OFF :
                if (other.id == k8090::CommandID::RELAY_ON) {
                    return true;
                }
                return false;
            default :
                return false;
        }
    }
    switch (id) {
        case k8090::CommandID::START_TIMER :
        case k8090::CommandID::SET_TIMER :
            for (int i = 1; i < 3; ++i) {
                if (params[i] != other.params[i]) {
                    return false;
                }
            }
            return true;
        case k8090::CommandID::TIMER :
            // compare only first bits
            if ((params[1] & 1) != (other.params[1] & 1)) {
                return false;
            }
            return true;
        default :
            return true;
    }
}

}  // namespace impl_
}  // namespace k8090
}  // namespace core
}  // namespace sprelay
