module ClockSync {
    @ to maintain an accurate record of time on the satellite.
    queued component TimeSync {

        ##################
        #### Commands ####
        ##################

        @ Set time from ground station (coarse sync)
        async command SET_TIME(
            seconds: U32,
            microseconds: U32
        ) opcode 0
        
        @ Enable/disable PPS synchronization
        async command ENABLE_PPS(
            enable: bool
        ) opcode 1
        
        @ Manual trigger for PPS sync (testing)
        sync command TRIGGER_PPS_SYNC opcode 2

        ################
        #### Ports ####
        ################

        @ PPS signal input from GPS component
        async input port ppsSignal: Svc.Cycle

        ###################
        #### Events ####
        ###################

        @ Time was updated via coarse sync
        event TimeCoarseSync(
            seconds: U32,
            microseconds: U32
        ) \
            severity activity high \
            format "Coarse sync: {} sec, {} usec"
        
        @ Time was synced via PPS
        event TimePPSSync(
            seconds: U32
        ) \
            severity activity low \
            format "PPS sync: {} sec"
        
        @ PPS sync drift detected
        event PPSDrift(
            driftMs: I32
        ) \
            severity warning high \
            format "PPS drift: {} ms"

        @ PPS synchronization enabled/disabled
        event PPSStateChanged(
            enabled: bool
        ) \
            severity activity high \
            format "PPS enabled: {}"

        @ Failed to set system time
        event TimeSetFailed(
            seconds: U32,
            microseconds: U32
        ) \
            severity warning high \
            format "Failed to set time: {} sec, {} usec"

        @ Failed to get system time
        event TimeGetFailed() \
            severity warning high \
            format "Failed to get system time"

        @ PPS device initialization failed
        event PPSInitFailed() \
            severity warning high \
            format "Failed to initialize PPS device"
        
        @ PPS timeout - no signal received
        event PPSTimeout() \
            severity warning low \
            format "PPS timeout: no signal received"

        ###################
        #### Telemetry ####
        ###################
        
        @ Last time synchronization occurred (seconds since epoch)
        telemetry LastSyncTime: U32
        
        @ Whether PPS synchronization is currently enabled
        telemetry PPSEnabled: bool
        
        @ Current drift from PPS in microseconds
        telemetry DriftMicroseconds: I32
        
        @ Total number of time syncs performed
        telemetry SyncCount: U32
        

        ###########################
        #### Standard AC Ports ####
        ###########################
        command recv port cmdIn
        command reg port cmdRegOut
        command resp port cmdResponseOut
        
        event port eventOut
        text event port textEventOut
        time get port timeGetOut
        telemetry port tlmOut

    }
}