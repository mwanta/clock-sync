// ======================================================================
// \title  TimeSync.cpp
// \author meredithwanta
// \brief  cpp file for TimeSync component implementation class
// ======================================================================

#include "ClockSync/Components/TimeSync/TimeSync.hpp"
#include <time.h>
#include <sys/time.h>
#include <Fw/Types/Assert.hpp>

namespace ClockSync {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

TimeSync::TimeSync(const char* const compName) 
    : TimeSyncComponentBase(compName),
      m_ppsEnabled(false),
      m_syncCount(0),
      m_lastSyncTime(0),
      m_driftMicroseconds(0)
{}

TimeSync::~TimeSync() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void TimeSync::ppsSignal_handler(FwIndexType portNum, Os::RawTime& cycleStart) {
    // Only process PPS if enabled
    if (!this->m_ppsEnabled) {
        return;
    }
    
    // Perform PPS synchronization
    this->performPPSSync();
}


// ----------------------------------------------------------------------
// Handler implementations for commands
// -

void TimeSync::SET_TIME_cmdHandler(
    FwOpcodeType opCode, 
    U32 cmdSeq, 
    U32 seconds, 
    U32 microseconds
) { 
    // Create timespec structure for setting time
    struct timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = microseconds * 1000; // Convert microseconds to nanoseconds
    
    // Set the system time
    if (clock_settime(CLOCK_REALTIME, &ts) == 0) {
        // Success - update telemetry and send event
        this->m_lastSyncTime = seconds;
        this->m_syncCount++;
        
        this->tlmWrite_LastSyncTime(seconds);
        this->tlmWrite_SyncCount(this->m_syncCount);
        
        this->log_ACTIVITY_HI_TimeCoarseSync(seconds, microseconds);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        // Failed to set time
        this->log_WARNING_HI_TimeSetFailed(seconds, microseconds);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

void TimeSync::ENABLE_PPS_cmdHandler(
    FwOpcodeType opCode, 
    U32 cmdSeq, 
    bool enable
) {
    // Update PPS enabled state
    this->m_ppsEnabled = enable;
    
    // Update telemetry
    this->tlmWrite_PPSEnabled(enable);
    
    // Send event
    this->log_ACTIVITY_HI_PPSStateChanged(enable);
    
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void TimeSync::TRIGGER_PPS_SYNC_cmdHandler(
    FwOpcodeType opCode, 
    U32 cmdSeq
) {
    // Manual trigger for testing - perform PPS sync regardless of enabled state
    this->performPPSSync();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Private helper methods
// ----------------------------------------------------------------------

void TimeSync::performPPSSync() {
    // Get current system time
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        this->log_WARNING_HI_TimeGetFailed();
        return;
    }
    
    // Calculate drift from whole second
    I32 driftNs = ts.tv_nsec;
    I32 driftUs = driftNs / 1000;
    
    // If drift is more than 500ms, round up to next second
    // Otherwise round down to current second
    if (ts.tv_nsec >= 500000000) {
        ts.tv_sec += 1;
    }

    // Set nanoseconds to 0 (synchronize to whole second)
    ts.tv_nsec = 0;
    
    // Set the system time
    if (clock_settime(CLOCK_REALTIME, &ts) == 0) {
        // Success - update telemetry and send event
        this->m_lastSyncTime = ts.tv_sec;
        this->m_syncCount++;
        this->m_driftMicroseconds = driftUs;
        
        this->tlmWrite_LastSyncTime(ts.tv_sec);
        this->tlmWrite_SyncCount(this->m_syncCount);
        this->tlmWrite_DriftMicroseconds(driftUs);
        
        this->log_ACTIVITY_LO_TimePPSSync(ts.tv_sec);
        
        // If drift is significant, log it
        I32 driftMs = driftUs / 1000;
        if (driftMs > 10 || driftMs < -10) {
            this->log_WARNING_HI_PPSDrift(driftMs);
        }
    } else {
        this->log_WARNING_HI_TimeSetFailed(ts.tv_sec, 0);
    }
}

}  // namespace ClockSync
