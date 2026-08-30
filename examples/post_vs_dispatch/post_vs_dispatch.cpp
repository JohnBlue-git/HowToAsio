#include <boost/asio.hpp>
#include <iostream>

int main() {
    std::cout << "=== post vs dispatch ===\n";

    boost::asio::io_context io;
    auto strand = boost::asio::make_strand(io);

    boost::asio::post(strand, [&] {
        std::cout << "outer handler: start\n";

        boost::asio::dispatch(strand, [] {
            std::cout << "dispatch: runs immediately when already on the strand\n";
        });

        boost::asio::post(strand, [] {
            std::cout << "post: queued and runs later\n";
        });

        std::cout << "outer handler: end\n";
    });

    io.run();
    std::cout << "done\n";
    return 0;
}
