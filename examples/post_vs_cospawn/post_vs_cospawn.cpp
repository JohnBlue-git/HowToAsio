#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>

int main() {
    std::cout << "=== post vs co_spawn ===\n";

    boost::asio::io_context io;

    std::cout << "main: enqueue several post callbacks to show the queue\n";
    boost::asio::post(io, [&io] {
        std::cout << "post #1: first item in the queue\n";
        boost::asio::post(io, [&io] {
            std::cout << "post #2: scheduled from post #1, still in the same queue\n";
            boost::asio::post(io, [] { std::cout << "post #3: final callback in the chain\n"; });
        });
    });

    boost::asio::post(io, [] {
        std::cout << "post #4: another task that is queued independently\n";
    });

    auto spawn_task = [&](std::string name, int delay_ms, std::function<void()> continuation) {
        boost::asio::co_spawn(
            io,
            [&, name, delay_ms, continuation]() -> boost::asio::awaitable<void> {
                std::cout << "co_spawn " << name << ": start\n";

                boost::asio::steady_timer timer{io.get_executor(), std::chrono::milliseconds(delay_ms)};
                co_await timer.async_wait(boost::asio::use_awaitable);

                std::cout << "co_spawn " << name << ": resume after wait\n";
                if (continuation) {
                    continuation();
                }

                co_return;
            },
            [](std::exception_ptr ex) {
                if (ex) {
                    try {
                        std::rethrow_exception(ex);
                    } catch (const std::exception& e) {
                        std::cerr << "co_spawn error: " << e.what() << '\n';
                    }
                }
            });
    };

    std::cout << "main: spawn multiple coroutines that link into each other\n";
    spawn_task("A", 30, [&]() {
        std::cout << "continuation from A: schedule B\n";
        spawn_task("B", 20, [&]() {
            std::cout << "continuation from B: schedule C and a plain post\n";
            spawn_task("C", 10, [] {
                std::cout << "continuation from C: final coroutine step\n";
            });

            boost::asio::post(io, [] {
                std::cout << "post #5: triggered while the coroutine chain is still active\n";
            });
        });
    });

    io.run();

    std::cout << "main: event loop finished; the queue and coroutine chain were processed in order\n";
    return 0;
}
