#include "simulator/system_thread.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stop_token>
#include <thread>

#include "systemwide.h"
#include "thermocycler-refresh/errors.hpp"
#include "thermocycler-refresh/tasks.hpp"

using namespace system_thread;

struct SimSystemPolicy {
  private:
    bool serial_number_set = false;
    static constexpr std::size_t SYSTEM_SERIAL_NUMBER_LENGTH =
        SYSTEM_WIDE_SERIAL_NUMBER_LENGTH;
    std::array<char, SYSTEM_SERIAL_NUMBER_LENGTH> system_serial_number = {};
    errors::ErrorCode set_serial_number_return = errors::ErrorCode::NO_ERROR;

  public:
    auto enter_bootloader() -> void { std::terminate(); }

    auto set_serial_number(
        std::array<char, SYSTEM_SERIAL_NUMBER_LENGTH> new_system_serial_number)
        -> errors::ErrorCode {
        // copy to system_serial_number
        auto copy_start = new_system_serial_number.begin();
        auto copy_length = static_cast<int>(new_system_serial_number.size());
        std::copy(copy_start, (copy_start + copy_length),
                  system_serial_number.begin());
        serial_number_set = true;
        return set_serial_number_return;
    }

    auto get_serial_number(void)
        -> std::array<char, SYSTEM_SERIAL_NUMBER_LENGTH> {
        if (serial_number_set) {
            return system_serial_number;
        } else {
            std::array<char, SYSTEM_SERIAL_NUMBER_LENGTH> empty_serial_number =
                {"EMPTYSN"};
            return empty_serial_number;
        }
    }
};

struct system_thread::TaskControlBlock {
    TaskControlBlock()
        : queue(SimSystemTask::Queue()), task(SimSystemTask(queue)) {}
    SimSystemTask::Queue queue;
    SimSystemTask task;
};

auto run(std::stop_token st, std::shared_ptr<TaskControlBlock> tcb) -> void {
    using namespace std::literals::chrono_literals;
    auto policy = SimSystemPolicy();
    tcb->queue.set_stop_token(st);
    while (!st.stop_requested()) {
        try {
            tcb->task.run_once(policy);
        } catch (const SimSystemTask::Queue::StopDuringMsgWait sdmw) {
            return;
        }
    }
}

auto system_thread::build()
    -> tasks::Task<std::unique_ptr<std::jthread>, SimSystemTask> {
    auto tcb = std::make_shared<TaskControlBlock>();
    return tasks::Task(std::make_unique<std::jthread>(run, tcb), &tcb->task);
}
