# ClockSync F´ project

## Summary:

The TimeSync component provides accurate time synchronization for satellite operations by managing the Linux system clock through F Prime's PosixTime component. This ensures all satellite subsystems (navigation, communications, mission management, and experiment execution) maintain synchronized, accurate time.

## Project Requirements:
Background: Maintaining accurate time is critical for satellite operations. Clocks drift over time and require periodic recalibration through two methods:

1. Coarse Synchronization - Ground station radio messages provide absolute timestamps
2. GPS PPS Synchronization - GPS Pulse Per Second signals provide precise second-boundary alignment



## Features:

### 1. Uses F Prime’s PosixTime component to keep track of the system time. Other components can successfully ping this component to get the system wide time.
  
#### Implementation:
- PosixTime Intregration: Deployed Svc.PosixTime component as the single source of truth for system time
- Topology Configuration: All components automatically query time via time connections instance posixTime pattern
- System Clock Management: TimeSync updates the Linux system clock via clock_settime(), which PosixTime reads and distributes
- All components are connected to PosixTime in topology.fpp

#### Workflow:
1. Ground/GPS sends timestamp/PPS signal to TimeSync
2. TimeSync updates Linux system clock via `clock_settime()`
3. PosixTime reads Linux system clock via `clock_gettime()`
4. All components query PosixTime for current time

```
                    Linux System Clock
                     (CLOCK_REALTIME)
                            ↑ ↓
                     write  │ │  read
                            │ │
        ┌──clock_settime()──┘ └──clock_gettime()──┐
        │                                         │
   TimeSync                                   PosixTime
        ↑                                         ↓
        │                                         │
   Ground/GPS                              All Components
```
### 2. Coarse synchronization: getting a mock radio message with a timestamp, and using this to calibrate the system time.

#### Implementation:
- Command SET_TIME(seconds, microseconds): accepts a timestamp from the ground station and sets Linux system clock
- Calibrates system time via clock_settime()
- Located in TimeSync.cpp
### 3. Implement PPS time sync: using the PPS value to more accurately reset the system time.

#### Implementation:
- Software PPS performPPSSync(): rounds time to the nearest second.
- Hardware PPS: Linux PPS subsystem integration with kernel level timestamps
- Triggers:
  - ppsSignal: async input port (recieves GPS PPS pulses)
  - TRIGGER_PPS_SYNC: command for manual testing
  - ENABLE_PPS: command to enable/disable PPS sync
 
## Component Setup:

```
ClockSync/Components/TimeSync/
├── TimeSync.fpp          # Component interface definition
├── TimeSync.hpp          # Component header
├── TimeSync.cpp          # Implementation
└── CMakeLists.txt        # Build configuration
ClockSyncDeployment
├── Main.cpp          
└── Top/topology.fpp
```

#### Commands:
- SET_TIME(seconds, microseconds): coarse time synchronization
- ENABLE_PPS(enable): enable/disable PPS synchronzation
- TRIGGER_PPS_SYNC: manual PPS sync trigger (testing)

#### Ports:
- ppsSignal(async input): recieves GPS PPS pulses

#### Events:
- TimeCoarseSync: logged when coarse sync executes
- TimePPSSync: logged when PPS sync executes
- PPSDrift: logged when significant drift is detected
- PPSStateChanged: logged when PPS enabled/disabled
- TimeSetFailed: error event for failed time updates
- TimeGetFailed: error event for failed time reads
- PPSInitFailed: error event for PPS initialization failure
- PPSTimeout: warning when PPS signal not recieved

#### Telemetry:
- LastSyncTime(U32): timestamp of last synchronization
- PPSEnabled(bool): current PPS enable state
- DriftMicroseconds(I32): measured time drift
- SyncCount(U32): total synchonizations performed

### GPS Hardware Integration To-Do's:
- Connect GPS module to Raspberry Pi
- Test Linux PPS subsystem integration
- Note that PPS related code in topology.fpp is currently commented out to allow for build testing

## Dependencies:

#### Hardware:
- Raspberry Pi
- GPS module with PPS output

#### Software:
- F' framework
- Linux operating system
- PPS kernal support
- pps-tools package (for testing)

#### Build Requirements:
- CMake 3.16+
- C++ compiler with C++14
- Python 3.6+ with fprime-tools

## Resources:
- PosixTime Implementation: nasa/fprime/Svc/PosixTime
- F Prime Tutorials: https://fprime.jpl.nasa.gov/
- Linux PPS Documentation: https://www.kernel.org/doc/html/latest/driver-api/pps.html


