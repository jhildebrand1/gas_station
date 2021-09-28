/**********************************************************************
*
* Gas Station Simulator
*
* This program simulates a gas station with 10 cars and 2 gas pumps
*
* Requirements:
* + 10 cars and 2 gas pumps
* + one car line waiting to use available pumps
* + each car spends 30 ms at the gas pump for one fill up
* + after fill up, car should get back in line
* + scenario runs for 30 seconds
* + count the number of fill ups per pump and per car
* + and print the results to stdout
* + cars should be represented as threads, not the pumps
* + the cars should initiate each action when its turn comes
*
* Compilation: g++ -std=c++11 -pthread gas_station.cpp and WSL
*
* Author: Jeff Hildebrand
* Contact: jhildebrand1@gmail.com
*
**********************************************************************/
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <ctime>
#include <string.h>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable> 

using std::cout;
using std::thread;
using std::vector;
using std::string;
using std::queue;
using std::atomic;

// Simple Logging Functions
#define LOG_ERROR(x ...) fprintf(stderr,x); fprintf(stderr,"\n");
#define LOG_DEBUG(x ...) if (0) { auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); \
                                  auto timestr = ctime(&now); timestr[strcspn(timestr, "\n")] = 0; \
                                  printf("%s [%s] ", timestr, __FUNCTION__); printf(x); printf("\n"); }
#define LOG_INFO(x ...) { printf(x); }

// Program Defines -- these could be arguments
static const int MAX_CARS = 10;
static const int MAX_PUMPS = 2;
static const int PROGRAM_TIME_S = 30; 
static const int PUMP_TIME_MS = 30;

/* thread-safe message queue */
template<class T>
class MyQueue
{
    public:
        void push(const T & item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(item);
        }
        T pop(void) 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            T rv = m_queue.front();
            m_queue.pop();
            return rv;
        }
        int front(void) 
        { 
            return (m_queue.front()); 
        }
        bool empty(void) { return m_queue.size()<=0; }
    private:
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;    
};

/* Base class to track and print stats */
class Metrics 
{
    public:
        Metrics(string s, int id) : m_name(s), m_id(id) { /* Do Nothing */ };
        ~Metrics(void) { if (m_count>0) PrintStats(); }
        void Inc(void) { ++m_count; }
        int GetId(void) { return m_id; }
    private:
        void PrintStats(void) { LOG_INFO( "%s %d filled up %d times\n", m_name.c_str(), m_id, m_count); }
        int m_id = 0;
        unsigned int m_count = 0;
        string m_name;
};

/* Gas Pump is basically just metrics */ 
class GasPump : public Metrics
{
    public:
        GasPump(int id) : Metrics("Pump", id) { }
        // Simulate time to pump gas by putting thread to sleep
        void PumpGas(void) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(PUMP_TIME_MS)); 
            Inc(); 
        }
};

/* Class to handle the car worker-thread */ 
/* ASSUMPTIONS: it is up to higher-level code to make each car-id unique, no error checking for this takes place */ 
class Car : public Metrics
{
    public:
        Car(int id) : Metrics("Car", id) { /* Do nothing */ }
        // Overload () for thread operation: this routine simulates what cars do at the gas station
        void operator()(const atomic<bool> & running, MyQueue<int> & line, MyQueue<GasPump*> & pumps)
        {
            // drive into line
            line.push(GetId());

            // run while station is open
            while (running.load()) {
                // Wait to see if we are in front
                if (line.front() == GetId()) {
                    // Check for available pumps - only one thread is checking at a time
                    while (pumps.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    // Leave the line only AFTER we get a pump
                    GasPump* myPump = pumps.pop();
                    if (myPump == nullptr) { LOG_ERROR("NULL PTR FOUND!"); return; }
                    line.pop();
                    myPump->PumpGas();
                    Inc(); // increment counts
                    pumps.push(myPump); // return pump back to the queue
                    line.push(GetId()); // Get back in line
                }
                // keep waiting...
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
};

/** Main Program Execution Starts Here **/
int main(int argc, char* argv[])
{
    LOG_INFO("Gas Station Simulator: %d cars, %d pumps, %dms fill-up time, and %ds total run time\n", MAX_CARS, MAX_PUMPS, PUMP_TIME_MS, PROGRAM_TIME_S);

    // Set up pumps and queues 
    atomic<bool> running(true); // program start/stop flag 
    MyQueue<int> carLine; // car line is represented as queue of Car IDs 
    MyQueue<GasPump*> openPumps; // queue of available pumps
    for (int i=0; i<MAX_PUMPS; ++i) {
        openPumps.push( new GasPump(i+1) );
    }

    // Start and keep track of threads
    vector<thread> cars;
    for (int i=0; i<MAX_CARS; ++i) {
        cars.push_back( thread(Car(i+1), std::ref(running), std::ref(carLine), std::ref(openPumps)) );
    }

    // Bound program execution time
    std::this_thread::sleep_for(std::chrono::seconds(PROGRAM_TIME_S));

    // Stop threads
    running = false;

    // Wait for car-threads to finish
    for (auto& t : cars) t.join();

    // cleanup pumps
    while (!openPumps.empty()) {
        GasPump *p = openPumps.pop();
        delete p;
    }

    LOG_DEBUG("Gas Station Simulator - End");

    return 0;
}
