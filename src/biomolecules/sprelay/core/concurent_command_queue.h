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
 * \file      concurent_command_queue.h
 * \brief     The biomolecules::sprelay::core::k8090::impl_::ConcurentCommandQueue class which specializes
 *            biomolecules::sprelay::core::command_queue::CommandQueue for usage in
 *            biomolecules::sprelay::core::k8090::K8090 class in multithreaded applications.
 *
 * \author    Jakub Klener <lumiksro@centrum.cz>
 * \date      2018-09-11
 * \copyright Copyright (C) 2018 Jakub Klener. All rights reserved.
 *
 * \copyright This project is released under the 3-Clause BSD License. You should have received a copy of the 3-Clause
 *            BSD License along with this program. If not, see https://opensource.org/licenses/.
 */


#ifndef BIOMOLECULES_SPRELAY_CORE_CONCURENT_COMMAND_QUEUE_H_
#define BIOMOLECULES_SPRELAY_CORE_CONCURENT_COMMAND_QUEUE_H_

#include "command_queue.h"
#include "k8090_commands.h"
#include "k8090_defines.h"
#include "k8090_utils.h"

namespace biomolecules {
namespace sprelay {
namespace core {
namespace k8090 {
namespace impl_ {

/// \brief Thread-safe version of command_queue::CommandQueue adapted for usage in K8090 class.
/// \headerfile ""
class ConcurentCommandQueue : private command_queue::CommandQueue<Command, as_number(k8090::CommandID::None)>
{
    using Predecessor = command_queue::CommandQueue<Command, as_number(k8090::CommandID::None)>;

public:  // NOLINT(whitespace/indent)
    bool empty() const;
    Command pop();
    unsigned int stampCounter() const;
    void updateOrPush(CommandID command_id, RelayID mask, unsigned char param1, unsigned char param2);
    int count(CommandID command_id) const;

private:  // NOLINT(whitespace/indent)
    bool updateCommandImpl(CommandID command_id, const Command& command);
    mutable std::mutex global_mutex_;
};

}  // namespace impl_
}  // namespace k8090
}  // namespace core
}  // namespace sprelay
}  // namespace biomolecules

#endif  // BIOMOLECULES_SPRELAY_CORE_CONCURENT_COMMAND_QUEUE_H_
