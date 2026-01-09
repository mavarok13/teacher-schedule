#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/asio.hpp>
#include <memory>

#include <http_client.hpp>
#include <html_parser.hpp>

namespace net = boost::asio;

int main() {
    net::io_context io_ctx;

    auto session = std::make_shared<http_client::Session>(io_ctx);
    session->Run("google.ru", "443", "/", "response.html", 11);

    io_ctx.run();

    std::ifstream ifs("response.html");

    std::stringstream buffer;
    buffer << ifs.rdbuf();

    const std::string url_regex = R"((https?://[^\s"'>]+))";
    auto links = html_parser::ExtractLinks(buffer.str(), url_regex);

    std::cout << "Found links:" << std::endl;
    for (const auto & link : links) {
        std::cout << link << std::endl;
    }

    return 0;
}
