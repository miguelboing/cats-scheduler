# C++ Scheduler

An algorithmic description in C++ of scheduling algorithms along with a simulator.

## Features

This project implements several classic CPU scheduling algorithms:

- **FCFS (First Come First Served)**: Tasks are executed in the order they arrive
- **SJF (Shortest Job First)**: Tasks with shorter execution time are prioritized
- **Priority Scheduling**: Tasks with higher priority are executed first
- **Round Robin**: Tasks are executed in time slices with preemption

## Project Structure

```
scheduler/
├── include/          # Header files
│   ├── scheduler.h   # Main scheduler class
│   └── simulator.h   # Simulation framework
├── src/              # Source files
│   ├── scheduler.cpp # Scheduler implementation
│   ├── simulator.cpp # Simulator implementation
│   └── main.cpp      # Demo application
├── tests/            # Test files
│   ├── simple_test.cpp      # Simple test runner
│   ├── test_scheduler.cpp   # GTest scheduler tests
│   └── test_simulator.cpp   # GTest simulator tests
└── CMakeLists.txt    # Build configuration
```

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- CMake 3.12 or newer
- Optional: Google Test (for advanced testing)

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/miguelboing/scheduler.git
   cd scheduler
   ```

2. **Create build directory:**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure and build:**
   ```bash
   cmake ..
   make
   ```

4. **Run the demo:**
   ```bash
   ./scheduler
   ```

5. **Run tests:**
   ```bash
   make test
   # or run directly:
   ./tests/simple_tests
   ```

## Usage

### Basic Usage

```cpp
#include "scheduler.h"

// Create scheduler with FCFS algorithm
scheduler::Scheduler sched(scheduler::Scheduler::Algorithm::FCFS);

// Add tasks
sched.addTask(std::make_unique<scheduler::Task>(1, 5, std::chrono::milliseconds(300)));
sched.addTask(std::make_unique<scheduler::Task>(2, 3, std::chrono::milliseconds(150)));

// Run scheduler
sched.run();

// Get results
std::cout << "Completed tasks: " << sched.getCompletedTaskCount() << std::endl;
std::cout << "Average wait time: " << sched.getAverageWaitTime() << " ms" << std::endl;
```

### Simulation

```cpp
#include "simulator.h"

// Configure simulation
scheduler::SimulationConfig config;
config.num_tasks = 10;
config.min_duration_ms = 100;
config.max_duration_ms = 500;

// Run simulation
scheduler::Simulator sim(config);
sim.compareAlgorithms();
```

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.
