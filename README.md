# ClockSync F´ project

**Summary:**

The TimeSync component provides accurate time synchronization for satellite operations by managing the Linux system clock through F Prime's PosixTime component. This ensures all satellite subsystems (navigation, communications, mission management, and experiment execution) maintain synchronized, accurate time.

**Project Requirements:**
Background: Maintaining accurate time is critical for satellite operations. Clocks drift over time and require periodic recalibration through two methods:

1. Coarse Synchronization - Ground station radio messages provide absolute timestamps
2. GPS PPS Synchronization - GPS Pulse Per Second signals provide precise second-boundary alignment



**Features:**

- Uses F Prime’s PosixTime component to keep track of the system time. Other components can successfully ping this component to get the system wide time.
Implementation
- Coarse synchronization: getting a mock radio message with a timestamp, and using this to calibrate the system time.
- Implement PPS time sync: using the PPS value to more accurately reset the system time.
