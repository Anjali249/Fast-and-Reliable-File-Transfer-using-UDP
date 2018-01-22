# Fast-and-Reliable-File-Transfer-using-UDP

MOTIVATION:
Even though TCP is effective, reliable and relatively robust on internet, it doesn’t always give us best throughput under various
circumstances, especially in the cases when the latency is very high and the link between the two nodes is very lossy. The basic design of TCP is that it implements congestion control mechanisms to ensure smooth flow of traffic. However, when losses occur due to whatsoever reason, TCP assumes that congestion has occurred and it drops its window size to half. Thus, when are losing 20% of the packets, within no time the TCP sender window drops to a very low value and our throughput is very bad and the transfer of data from source to destination takes a very long time. 

GOAL:
The aim of this project is to design an UDP based file transfer utility which can fulfill three basic requirements: 
Uses IP (so it can be routed) 
Implemented with command line interface like scp
Ensures reliability (with no errors) to transfer high amount of file data on a link that has high latency and is very lossy.

Test Platform
DETERLab

