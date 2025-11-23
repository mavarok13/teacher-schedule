#include <iostream>
#include <memory>
#include <functional>

#include <xlnt/xlnt.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/xpressive/xpressive.hpp>

#include "../TeacherSchedule/src/http_client.hpp"


namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace boost::xpressive;

using Request = http::request<http::string_body>;
using Response = http::response<http::string_body>;
using RequestHandler = std::function<void(Response)>;

int main() {
    setlocale(LC_ALL, "ru_RU");

    boost::urls::url_view uv{"https://rguk.ru/students/schedule/"};
    boost::urls::url url = uv;

    net::io_context io;

    net::ssl::context ssl_ctx{net::ssl::context::sslv23};
    ssl_ctx.set_default_verify_paths();

    http::request<http::string_body> req{http::verb::get, url.path(), 11};
    req.set(http::field::host, url.host());
    req.set(http::field::accept, "text/plain");
    req.set(http::field::user_agent, "Boost");

    boost::system::error_code ec;
    http::response<http::string_body> res;

    auto client = http_client::HttpClient<Request, Response, RequestHandler>::Create(io, url, ssl_ctx, std::move(req), std::move(res), [] (Response res) {
        std::string html_text = res.body().data();
        
        sregex re = sregex::compile("\"(?!https?://)[^\"]*\\.xlsx\"");

        sregex_iterator words_begin(html_text.begin(), html_text.end(), re);
        sregex_iterator words_end;

        if (words_begin == words_end) {
            std::cout << "Ссылок не найдено.\n";
        } else {
            std::cout << "Найденные ссылки:\n";
            for (auto it = words_begin; it != words_end; ++it) {
                std::cout << it->str() << std::endl;
            }
        }
    });

    client->SendRequest();

    io.run();

    return 0;
}