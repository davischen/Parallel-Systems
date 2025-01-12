#include "spin_barrier.h"
#include <thread>
#include <chrono>
#include <iostream>

// SpinLock Constructor
SpinLock::SpinLock() : flag(ATOMIC_FLAG_INIT), retries(0) {}

// Acquire SpinLock with improved backoff
void SpinLock::lock() {
    retries = 0;
    while (flag.test_and_set(std::memory_order_acquire)) {
        backoff();  // Retry with backoff
        retries++;
        if (retries > 100000) {  // Add a retry limit to detect failure
            std::cerr << "Error: SpinLock acquisition failed after excessive retries." << std::endl;
            std::exit(EXIT_FAILURE);  // Exit on failure
        }
    }
}

// Release SpinLock
void SpinLock::unlock() {
    flag.clear(std::memory_order_release);
}

// Improved backoff mechanism with exponential backoff and millisecond sleep
void SpinLock::backoff() {
    const int max_retries = 10;
    if (retries < max_retries) {
        std::this_thread::yield();  // Yield CPU after a few retries
    } else {
        auto delay = std::chrono::milliseconds(1 << (retries - max_retries));  // Exponential backoff in milliseconds
        std::this_thread::sleep_for(delay);  // Sleep to reduce CPU contention
    }
}

// spin_barrier Constructor
spin_barrier::spin_barrier(int num_threads) : num_threads(num_threads), count(0), phase(0) {}

// Wait function for barrier synchronization with hybrid spinning + millisecond sleeping
void spin_barrier::wait() {
    lock.lock();  // Acquire the SpinLock
    int current_phase = phase;  // Save the current phase

    if (++count == num_threads) {
        count = 0;  // Reset count for next use
        phase++;    // Move to the next synchronization phase
        lock.unlock();  // Release the SpinLock
    } else {
        lock.unlock();  // Release the SpinLock
        // Busy-wait with limited spinning and sleeping
        int spin_retries = 0;
        while (phase == current_phase) {
            if (++spin_retries < 1000) {
                std::this_thread::yield();  // Yield for short wait
            } else if (spin_retries > 100000) {  // Add a limit to detect potential failure
                std::cerr << "Error: spin_barrier wait failed after excessive retries." << std::endl;
                std::exit(EXIT_FAILURE);  // Exit on failure
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Sleep for 1 millisecond for longer waits
            }
        }
    }
}