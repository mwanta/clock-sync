// ======================================================================
// \title  TimeSync.hpp
// \author meredithwanta
// \brief  hpp file for TimeSync component implementation class
// ======================================================================

#ifndef ClockSync_TimeSync_HPP
#define ClockSync_TimeSync_HPP

#include "ClockSync/Components/TimeSync/TimeSyncComponentAc.hpp"

namespace ClockSync {

class TimeSync final : public TimeSyncComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct TimeSync object
    TimeSync(const char* const compName  //!< The component name
    );

    //! Destroy TimeSync object
    ~TimeSync();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    ///! Handler for ppsSignal
    void ppsSignal_handler(
        FwIndexType portNum,
        Os::RawTime& cycleStart
    ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler for command SET_TIME
    void SET_TIME_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        U32 seconds,
        U32 microseconds
    ) override;

    //! Handler for command ENABLE_PPS
    void ENABLE_PPS_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        bool enable
    ) override;

    //! Handler for command TRIGGER_PPS_SYNC
    void TRIGGER_PPS_SYNC_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

  private:
    // ----------------------------------------------------------------------
    // Private helper methods
    // ----------------------------------------------------------------------
    //! Perform PPS synchronization
    void performPPSSync();

  private:
    // ----------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------
    //! Whether PPS synchronization is enabled
    bool m_ppsEnabled;

    //! Total count of synchronizations performed
    U32 m_syncCount;

    //! Last synchronization time (seconds since epoch)
    U32 m_lastSyncTime;

    //! Last measured drift in microseconds
    I32 m_driftMicroseconds;
};

}  // namespace ClockSync

#endif