//
// Copyright © 2020 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "ActivateTimelineReportingCommandHandler.hpp"
#include "TimelineUtilityMethods.hpp"

#include <armnn/Exceptions.hpp>
#include <boost/format.hpp>

namespace armnn
{

namespace profiling
{

void ActivateTimelineReportingCommandHandler::operator()(const Packet& packet)
{
    ProfilingState currentState = m_StateMachine.GetCurrentState();

    if (!m_ReportStructure.has_value())
    {
            throw armnn::Exception(std::string("Profiling Service constructor must be initialised with an "
                                               "IReportStructure argument in order to run timeline reporting"));
    }

    switch ( currentState )
    {
        case ProfilingState::Uninitialised:
        case ProfilingState::NotConnected:
        case ProfilingState::WaitingForAck:
            throw RuntimeException(boost::str(
                boost::format("Activate Timeline Reporting Command Handler invoked while in a wrong state: %1%")
                    % GetProfilingStateName(currentState)));
        case ProfilingState::Active:
            if ( !( packet.GetPacketFamily() == 0u && packet.GetPacketId() == 6u ))
            {
                throw armnn::Exception(std::string("Expected Packet family = 0, id = 6 but received family =")
                                           + std::to_string(packet.GetPacketFamily())
                                           + " id = " + std::to_string(packet.GetPacketId()));
            }

            m_SendTimelinePacket.SendTimelineMessageDirectoryPackage();

            TimelineUtilityMethods::SendWellKnownLabelsAndEventClasses(m_SendTimelinePacket);

            m_TimelineReporting = true;

            m_ReportStructure.value().ReportStructure();

            m_BackendNotifier.NotifyBackendsForTimelineReporting();

            break;
        default:
            throw RuntimeException(boost::str(boost::format("Unknown profiling service state: %1%")
                                                  % static_cast<int>(currentState)));
    }
}

} // namespace profiling

} // namespace armnn