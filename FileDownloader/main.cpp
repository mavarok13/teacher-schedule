#include <iostream>

#include "http_client.hpp"

int main() {
    const std::string host = "www.google.ru";
    const std::string port = "443";
    const std::string target = "/";

    net::io_context io;
    std::make_shared<http_client::Session>(io)->Run(host, port, target, "output.html", 11);
    io.run();

    return 0;
}