# ADAPTIVE-TRAFFIC-MANAGEMENT-SYSTEM

Adaptive Traffic Management System
-This project is a simple simulation of a smart traffic control system for a 4-way intersection.
-Instead of using fixed timers, it adjusts traffic lights based on how many vehicles are waiting in each lane.
-It also keeps track of system data using SQLite and logs each cycle so you can analyze what’s happening over time.


What this system does
1. Traffic control
-Chooses the busiest lane automatically
-Gives green light based on traffic load
-Reduces vehicles each cycle to simulate movement
-Tracks waiting time for all lanes
2. Emergency handling
-If a lane gets too crowded (40+ vehicles), it gets priority immediately
-Overrides normal lane selection logic


Operating modes
1. Auto Mode
-Runs continuously on its own
-Executes cycles automatically
-Pauses every 5 cycles so you can check progress
-Stops when all lanes are empty
2. Manual Mode
-You enter traffic data every cycle
-System runs step by step based on your input


How it works (simple version)
-The system checks all lanes
-Picks the one with the most vehicles (unless emergency happens)
-Assigns green light time based on traffic:
-Heavy traffic → longer green time
-Light traffic → shorter green time
-Vehicles are reduced each cycle to simulate movement
-Waiting times increase for lanes that are not selected

Analytics
The system also stores and shows data using SQLite:
-Full traffic logs for each cycle
-Maximum vehicles recorded per lane
-Number of emergency events
There’s also a cycle_log.txt file that records each cycle in a simple format.

Data storage
1. SQLite database
traffic_log → history of all cycles
current_state → latest lane status
2. File logging
cycle_log.txt → readable cycle-by-cycle logs


Tech used
1. C++
2. SQLite3
3. File handling (fstream)
4. Console-based interface
