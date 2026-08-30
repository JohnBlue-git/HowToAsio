#include <boost/asio.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::cout << "=== io_context vs strand ===\n";

    {
        std::cout << "\n1) Without strand: handlers can overlap\n";
        boost::asio::io_context io;
        std::atomic<int> counter{0};

        for (int i = 0; i < 4; ++i) {
            boost::asio::post(io, [&counter] {
                const int id = ++counter;
                std::cout << "naive task " << id << " start\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
                std::cout << "naive task " << id << " end\n";
            });
        }

        std::vector<std::thread> workers;
        workers.reserve(2);
        for (int i = 0; i < 2; ++i) {
            workers.emplace_back([&io] { io.run(); });
        }
        for (auto& worker : workers) {
            worker.join();
        }
    }

    {
        std::cout << "\n2) With strand: handlers are serialized\n";
        boost::asio::io_context io;
        auto strand = boost::asio::make_strand(io);
        std::atomic<int> counter{0};

        for (int i = 0; i < 4; ++i) {
            boost::asio::post(strand, [&counter] {
                const int id = ++counter;
                std::cout << "strand task " << id << " start\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
                std::cout << "strand task " << id << " end\n";
            });
        }

        std::vector<std::thread> workers;
        workers.reserve(2);
        for (int i = 0; i < 2; ++i) {
            workers.emplace_back([&io] { io.run(); });
        }
        for (auto& worker : workers) {
            worker.join();
        }
    }

    std::cout << "\nThe key difference is that a strand guarantees serialized execution, while a plain io_context does not.\n";
    return 0;
}
