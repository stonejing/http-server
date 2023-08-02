#include <iostream>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>

class TimeWheel {
private:
    using TimerCallback = std::function<void()>;

    struct TimerEntry {
        TimerCallback callback;
        int duration; // Timer duration in milliseconds
        int rotations; // Number of rotations remaining before the timer fires
    };

    static constexpr int wheelSize = 60; // Number of slots in the time wheel
    static constexpr int tickInterval = 1000; // Tick interval in milliseconds (1 second)
    std::vector<std::vector<TimerEntry>> timeWheel;
    int currentSlot;

public:
    TimeWheel() : timeWheel(wheelSize), currentSlot(0) {}

    void addTimer(int duration, TimerCallback callback) {
        int slotsToAdvance = duration / tickInterval;
        int targetSlot = (currentSlot + slotsToAdvance) % wheelSize;

        timeWheel[targetSlot].push_back({callback, duration, 0});
    }

    void tick() {
        std::vector<TimerEntry>& currentSlotTimers = timeWheel[currentSlot];
        for (auto& timerEntry : currentSlotTimers) {
            if (timerEntry.rotations == 0) {
                // Call the timer callback
                timerEntry.callback();
                timerEntry.rotations = timerEntry.duration / tickInterval;
            }
            timerEntry.rotations--;
        }

        // Move to the next slot
        currentSlot = (currentSlot + 1) % wheelSize;
    }

    void start() {
        while (true) {
            tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(tickInterval));
        }
    }
};