// Farha Ferdous
// CSCI 340 Project 3

#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <sstream>
#include <climits>

using namespace std;

struct Process {
    int id;          // process ID
    int arrival;     // arrival time
    int burst;       // original burst time
    int priority;    // priority (lower number = higher priority)
    int remaining;   // remaining burst time (for preemptive algorithms)
    int finish;      // finish time (-1 if not completed)
};

// ---reading processes from input file---
vector<Process> readProcesses(const string& filename) {
    vector<Process> processes;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        vector<string> tokens;

        while (getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() == 4) {   // make sure command has 4 args
            Process p;
            p.id = stoi(tokens[0]); // process id
            p.arrival = stoi(tokens[1]);    // arrival time
            p.burst = stoi(tokens[2]);  // burst time
            p.priority = stoi(tokens[3]);   // priority
            p.remaining = p.burst;  // remaining time
            p.finish = -1;  // not complered
            processes.push_back(p);
        }
    }

    return processes;
}

// ---printing all processes in order of ID---
void printProcessInfo(const vector<Process>& processes) {
    for (const auto& p : processes) {
        cout << p.id << "," << p.arrival << "," << p.finish << "," 
             << (p.finish - p.arrival) << endl; // turnaround time
    }
}

// ---first come first serve (non-preemptive)---
void fcfs(vector<Process> processes) {
    int currentTime = 0;
    for (auto& p : processes) {
        if (currentTime < p.arrival) {  // if no process is readt jumps to arrival time
            currentTime = p.arrival;
        }
        currentTime += p.burst;
        p.finish = currentTime;
    }
    printProcessInfo(processes);    // print results in ID order
}

// ---shortest remaining time first (SRTF) (preemptive)---
void srtf(vector<Process> processes) {
    int currentTime = 0;
    int completed = 0;
    int n = processes.size();
    vector<int> remainingTime(n);
    vector<bool> isCompleted(n, false);

    for (int i = 0; i < n; i++) {
        remainingTime[i] = processes[i].burst;
    }
    // main scheduling loop
    while (completed != n) {
        int shortest = -1;  // index of shortest remaining process
        int minRemaining = INT_MAX; // track smallest remaining time

        // finding process with shortest remaining time
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival <= currentTime && !isCompleted[i] && 
                remainingTime[i] < minRemaining) {
                minRemaining = remainingTime[i];
                shortest = i;
            }
        }
        // in crementing time if no process ready
        if (shortest == -1) {
            currentTime++;
            continue;
        }
        // executing process for 1 time unit
        remainingTime[shortest]--;
        currentTime++;
        // checking if process complee
        if (remainingTime[shortest] == 0) {
            processes[shortest].finish = currentTime;
            isCompleted[shortest] = true;
            completed++;
        }
    }
    printProcessInfo(processes);
}

// ---priority scheduling (preemptive)---
void priorityScheduling(vector<Process> processes) {
    int currentTime = 0;
    int completed = 0;
    int n = processes.size();
    vector<int> remainingTime(n);
    vector<bool> isCompleted(n, false);

    for (int i = 0; i < n; i++) {
        remainingTime[i] = processes[i].burst;
    }

    while (completed != n) {
        int highestPriority = -1;   // highest priority
        int minPriority = INT_MAX;  // tracking highest priority = smallest number

        // finding highest priority process
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival <= currentTime && !isCompleted[i] && 
                processes[i].priority < minPriority) {
                minPriority = processes[i].priority;
                highestPriority = i;
            }
        }

        if (highestPriority == -1) {
            currentTime++;
            continue;
        }

        remainingTime[highestPriority]--;
        currentTime++;

        if (remainingTime[highestPriority] == 0) {
            processes[highestPriority].finish = currentTime;
            isCompleted[highestPriority] = true;
            completed++;
        }
    }
    printProcessInfo(processes);    // print in order of ID
}

// ---round robin scheduling (RR)---
void roundRobin(vector<Process> processes, int quantum) {
    int currentTime = 0;
    int n = processes.size();
    vector<int> remainingTime(n);
    vector<bool> isCompleted(n, false);
    queue<int> readyQueue;
    vector<bool> inQueue(n, false);

    for (int i = 0; i < n; i++) {
        remainingTime[i] = processes[i].burst;
    }

    int nextProcess = 0;    // index of next arriving process

    while (true) {
        // adding arriving processes to queue
        while (nextProcess < n && processes[nextProcess].arrival <= currentTime) {
            if (!inQueue[nextProcess]) {
                readyQueue.push(nextProcess);
                inQueue[nextProcess] = true;
            }
            nextProcess++;
        }

        // handling empty queue case
        if (readyQueue.empty()) {
            if (nextProcess < n) {
                currentTime = processes[nextProcess].arrival;   // jump to next arrial time
                continue;
            } else {
                break;  // all processes complete
            }
        }
        // next process from queue
        int current = readyQueue.front();
        readyQueue.pop();
        inQueue[current] = false;

        // deteemining remaining time after time quantim
        int timeSlice = min(quantum, remainingTime[current]);
        remainingTime[current] -= timeSlice;
        currentTime += timeSlice;

        // checking for new arrivals dring time slice
        while (nextProcess < n && processes[nextProcess].arrival <= currentTime) {
            if (!inQueue[nextProcess]) {
                readyQueue.push(nextProcess);
                inQueue[nextProcess] = true;
            }
            nextProcess++;
        }
        // handling process completion or reqeueue
        if (remainingTime[current] > 0) {
            readyQueue.push(current);
            inQueue[current] = true;
        } else {
            processes[current].finish = currentTime;    // process completed
            isCompleted[current] = true;
        }
    }
    printProcessInfo(processes);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <test file> <algorithm> [quantum]" << endl;
        return 1;
    }

    string filename = argv[1];  // filename input
    string algorithm = argv[2]; // schedling algo
    int quantum = 0;

    // round robin args
    if (algorithm == "rr" && argc < 4) {
        cerr << "Error: Round Robin requires a time quantum" << endl;
        return 1;
    }

    if (algorithm == "rr") {
        quantum = stoi(argv[3]);    // get quant value
    }

    // read processesfrom input file
    vector<Process> processes = readProcesses(filename);
    // which algo to execute
    if (algorithm == "fcfs") {
        fcfs(processes);
    } else if (algorithm == "srtf") {
        srtf(processes);
    } else if (algorithm == "pri") {
        priorityScheduling(processes);
    } else if (algorithm == "rr") {
        roundRobin(processes, quantum);
    } else {
        cerr << "Error: Invalid scheduling algorithm" << endl;
        return 1;
    }

    return 0;
}