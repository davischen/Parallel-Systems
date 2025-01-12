#ifndef SPIN_BARRIER_H
#define SPIN_BARRIER_H

#include <atomic>

class SpinLock {
public:
    SpinLock();

    void lock();
    void unlock();

private:
    void backoff();  // Improved backoff
    std::atomic_flag flag;
    int retries;
};

// Class representing the spin barrier
class spin_barrier {
public:
    // Constructor
    spin_barrier(int num_threads);

    // Wait function for threads to synchronize at the barrier
    void wait();

private:
    SpinLock lock;           // SpinLock for protecting shared data
    int num_threads;         // Number of threads required to synchronize
    std::atomic<int> count;  // Count of threads that reached the barrier
    std::atomic<int> phase;  // Phase for managing barriers
};

#endif // SPIN_BARRIER_H
