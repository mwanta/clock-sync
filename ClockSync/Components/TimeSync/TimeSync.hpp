// ======================================================================
// \title  TimeSync.hpp
// \author meredithwanta
// \brief  hpp file for TimeSync component implementation class
// ======================================================================

#ifndef ClockSync_TimeSync_HPP
#define ClockSync_TimeSync_HPP

#include "ClockSync/Components/TimeSync/TimeSyncComponentAc.hpp"
#include <Os/Task.hpp>

// Only include PPS headers on Linux
#if defined(__linux__) && !defined(__APPLE__)
  #define HAVE_LINUX_PPS 1
  #include <sys/timepps.h>
#endif

namespace ClockSync {

class TimeSync final : public TimeSyncComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct TimeSync object
    TimeSync(const char* const compName
    );

    //! Destroy TimeSync object
    ~TimeSync();

    //! Initialize PPS device w/ it's path
    //! \param ppsDevice Path to PPS device (e.g. "/dev/pps0")
    //! \return true if initialization successful, false if failed
    bool initPPS(const char* ppsDevice);

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

#ifdef HAVE_LINUX_PPS
    //! PPS monitoring thread entry point
    static void ppsThreadEntry(void* ptr);

    //! PPS monitoring loop (runs in separate thread)
    void ppsMonitorLoop();
#endif
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

#ifdef HAVE_LINUX_PPS
    //! PPS device file descriptor
    int m_ppsFd;

    //! PPS handle for kernel PPS subsystem
    pps_handle_t m_ppsHandle;

    //! PPS monitoring thread
    Os::Task m_ppsTask;

    //! Flag to stop PPS thread
    volatile bool m_stopPpsThread;
#endif
};

}  // namespace ClockSync

#endif