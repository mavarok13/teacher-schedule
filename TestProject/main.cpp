#include <iostream>
#include <memory>
#include <functional>

#include <xlnt/xlnt.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include "../TeacherSchedule/src/http_client.hpp"


namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using Request = http::request<http::string_body>;
using Response = http::response<http::file_body>;
using RequestHandler = std::function<void(Response)>;

int main () {
    setlocale(LC_ALL, "ru_RU");

    boost::urls::url_view uv{"https://mgsu.ru/student/Raspisanie_zanyatii_i_ekzamenov/raspisanie_ekzamenovN/IGES_IV_1-13_s.XLS"};
    boost::urls::url url = uv;
    
    net::io_context io;

    net::ssl::context ssl_ctx{net::ssl::context::sslv23};
    ssl_ctx.set_default_verify_paths();

    http::request<http::string_body> req{http::verb::get, url.path(), 11};
    req.set(http::field::host, url.host());
    req.set(http::field::accept, "*/*");
    req.set(http::field::user_agent, "Boost");
    
    boost::system::error_code ec;
    http::response<http::file_body> res;
    res.body().open("testfile.xlsx", beast::file_mode::write, ec);

    auto client = http_client::HttpClient<Request, Response, RequestHandler>::Create(io, url, ssl_ctx, std::move(req), std::move(res), [] (Response res) {
        std::cout << "File downloaded" << std::endl;
    });

    client->SendRequest();

    io.run();

    return 0;
}