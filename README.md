# gas_station
Gas Station Simulator to test Multi-Threading using C++

Requirements:
+ 10 cars and 2 gas pumps
+ one car line waiting to use available pumps
+ each car spends 30 ms at the gas pump for one fill up
+ after fill up, car should get back in line
+ scenario runs for 30 seconds
+ count the number of fill ups per pump and per car
+ and print the results to stdout
+ cars should be represented as threads, not the pumps
+ the cars should initiate each action when its turn comes

Compilation: g++ -std=c++11 -pthread gas_station.cpp and WSL

Author: Jeff Hildebrand
Contact: jhildebrand1@gmail.com
