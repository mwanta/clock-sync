# ClockSync F´ project

**Summary:**

Maintaining an accurate record of the time on our satellite is crucial to many of its functionalities: from navigation, to communications, to mission management and experiment execution. As a result, our satellite will have a dedicated, always-on, real time clock.

On Linux systems, such as the one we will be using for our satellite, we can leverage F Prime’s Posix time component to manage this capability. This enables us to have a single source of truth for timing: whenever another component needs the time, it’ll just ping this component.

However, clocks go out of sync, and need to be recalibrated. A crucial method of doing this recalibration is GPS PPS. PPS or Pulse Per second, is a timing signal provided by GPS satellites that enables us to calibrate to the nearest whole second: this is useful if we are a few milliseconds off. 

However, in a worst case we may need to perform coarse synchronization: getting the actual timestamp for a radio message from the ground in order to recalibrate.

The goal for this ticket is to implement this PosixTime component, and implement both forms of synchronization. 

**Features:**

- Implement the ability to keep track of the system time, using F Prime’s PosixTime component. Ensure that other components can successfully ping this component to get the system wide time.
- Implement coarse synchronization: getting a mock radio message with a timestamp, and using this to calibrate the system time.
- Implement PPS time sync: using the PPS value to more accurately reset the system time.
