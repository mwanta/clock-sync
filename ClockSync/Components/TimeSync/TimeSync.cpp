// ======================================================================
// \title  TimeSync.cpp
// \author meredithwanta
// \brief  cpp file for TimeSync component implementation class
// ======================================================================

#include "ClockSync/Components/TimeSync/TimeSync.hpp"
#include <time.h>
#include <sys/time.h>
#include <sys/timepps.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
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
      m_driftMicroseconds(0),
      m_ppsFd(-1),
      m_stopPpsThread(false)
{}

TimeSync::~TimeSync() {
    // Stop PPS thread
    this->m_stopPpsThread = true;
    
    // Wait a bit for thread to exit
    Os::Task::delay(100);
    
    // Clean up PPS resources
    if (this->m_ppsFd >= 0) {
        time_pps_destroy(this->m_ppsHandle);
        close(this->m_ppsFd);
        this->m_ppsFd = -1;
    }
}

bool TimeSync::initPPS(const char* ppsDevice) {
    // Open PPS device
    this->m_ppsFd = open(ppsDevice, O_RDWR);
    if (this->m_ppsFd < 0) {
        this->log_WARNING_HI_PPSInitFailed();
        return false;
    }
    
    // Create PPS handle
    if (time_pps_create(this->m_ppsFd, &this->m_ppsHandle) < 0) {
        this->log_WARNING_HI_PPSInitFailed();
        close(this->m_ppsFd);
        this->m_ppsFd = -1;
        return false;
    }
    
    // Set PPS parameters - capture rising edge (assert)
    pps_params_t params;
    params.mode = PPS_CAPTUREASSERT | PPS_OFFSETASSERT;
    params.assert_offset.tv_sec = 0;
    params.assert_offset.tv_nsec = 0;

    if (time_pps_setparams(this->m_ppsHandle, &params) < 0) {
        this->log_WARNING_HI_PPSInitFailed();
        time_pps_destroy(this->m_ppsHandle);
        close(this->m_ppsFd);
        this->m_ppsFd = -1;
        return false;
    }
    
    // Start PPS monitoring thread
    Os::Task::TaskStatus stat = this->m_ppsTask.start(
        "PPS_Monitor",
        ppsThreadEntry,
        this,
        Os::Task::TASK_DEFAULT,
        16 * 1024  // 16KB stack
    );
    
    if (stat != Os::Task::TASK_OK) {
        this->log_WARNING_HI_PPSInitFailed();
        time_pps_destroy(this->m_ppsHandle);
        close(this->m_ppsFd);
        this->m_ppsFd = -1;
        return false;
    }

    return true;
}


// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void TimeSync::ppsSignal_handler(FwIndexType portNum, Os::RawTime& cycleStart) {
    // This handler is for software PPS signals from other components
    // When using Linux PPS subsystem, use the kernel timestamps instead
    // USE: backwards compatibility or software-only mode
    
    if (!this->m_ppsEnabled) {
        return;
    }
    
    // If hardware PPS is not available, use software sync
    if (this->m_ppsFd < 0) {
        this->performPPSSync();
    }
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
    // Get current system time (software fallback)
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

void TimeSync::ppsThreadEntry(void* ptr) {
    TimeSync* comp = static_cast<TimeSync*>(ptr);
    comp->ppsMonitorLoop();
}

void TimeSync::ppsMonitorLoop() {
    pps_info_t info;
    struct timespec timeout;
    
    while (!this->m_stopPpsThread) {
        // Only process if PPS is enabled and device is open
        if (!this->m_ppsEnabled || this->m_ppsFd < 0) {
            Os::Task::delay(1000); // Sleep 1 second if disabled
            continue;
        }
        
        // Wait up to 3 seconds for PPS event
        timeout.tv_sec = 3;
        timeout.tv_nsec = 0;
        
        int ret = time_pps_fetch(
            this->m_ppsHandle,
            PPS_TSFMT_TSPEC,
            &info,
            &timeout
        );

        if (ret == 0) {
            // Got PPS event with kernel timestamp!
            // This timestamp is captured by the kernel at interrupt time
            // giving us microsecond accuracy
            struct timespec ts = info.assert_timestamp;
            
            // Calculate drift in microseconds
            I32 driftUs = ts.tv_nsec / 1000;
            
            // The PPS pulse marks the exact second boundary
            // Truncate to whole second
            ts.tv_nsec = 0;
            
            // Set system time using kernel-captured timestamp
            if (clock_settime(CLOCK_REALTIME, &ts) == 0) {
                // Update telemetry
                this->m_lastSyncTime = ts.tv_sec;
                this->m_syncCount++;
                this->m_driftMicroseconds = driftUs;
                
                this->tlmWrite_LastSyncTime(ts.tv_sec);
                this->tlmWrite_SyncCount(this->m_syncCount);
                this->tlmWrite_DriftMicroseconds(driftUs);

                this->log_ACTIVITY_LO_TimePPSSync(ts.tv_sec);
                
                // Log significant drift (more than 10ms)
                I32 driftMs = driftUs / 1000;
                if (driftMs > 10 || driftMs < -10) {
                    this->log_WARNING_HI_PPSDrift(driftMs);
                }
            } else {
                this->log_WARNING_HI_TimeSetFailed(ts.tv_sec, 0);
            }
        } else if (errno == ETIMEDOUT) {
            // Timeout - no PPS signal received in 3 seconds
            this->log_WARNING_LO_PPSTimeout();
        }
        // Other errors are silently ignored (device might not be ready yet)
    }
}

}  // namespace ClockSync
