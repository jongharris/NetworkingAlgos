CPSC 441 - University of Calgary
Routing Tradeoffs - 40/40 marks recieved

Specification:
Your task is to write a program in C or C++ that can simulate the network routing algorithms below, and estimate the call blocking performance for a circuit-switched network.

In essence, this assignment is a discrete-event network simulation problem. Think of a loop that models the arrival and/or departure of each call over time, and using this loop to track the usage of each network link. Calls arrive at certain times, based on the input file, and depart at certain times, based on their durations. Specifically, your program should be able to compute the call routing performance metrics for different routing algorithms, when provided a specified call workload file (i.e., traffic load for the network) and a specified network topology.

Shortest Hop Path First (SHPF): This algorithm tries to find the shortest path currently available from source to destination, where the length of a path refers to the number of hops (i.e., links) traversed. Note that this algorithm ignores the propagation delay associated with each link.
Shortest Delay Path First (SDPF): This algorithm tries to find the shortest path currently available from source to destination, where the length of a path refers to the cumulative total propagation delay for traversing the chosen links in the path. Note that this algorithm ignores the number of hops.
Least Loaded Path (LLP): This algorithm tries to find the least loaded path currently available from source to destination, where the load of a path is determined by the "busiest" link on the path, and the load on a link is its current utilization (i.e., the ratio of its current number of active calls to the capacity C of that link for carrying calls). Note that this algorithm ignores propagation delays and the number of hops.
Maximum Free Circuits (MFC): This algorithm tries to find the currently available path from source to destination that has the most free circuits, where the number of free circuits for a candidate path is the smallest number of free circuits currently observed on any link in that possible path. Note that this algorithm ignores propagation delays, the number of hops, and the utilization of each link.
