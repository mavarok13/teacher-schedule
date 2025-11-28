#pragma once

#include <iostream>
#include <memory>

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

namespace net = boost::asio;
namespace sys = boost::system;
namespace beast = boost::beast;
namespace ssl = net::ssl;
namespace http = beast::http;
using tcp = net::ip::tcp;
using namespace boost::urls;

namespace http_client {

class Session : public std::enable_shared_from_this<Session> {
public:
    Session (net::io_context & io) : io_{io}, resolver_{io}, stream_{io, ssl_ctx_} {

    }

    Session (net::io_context & io, int redirect_count)
        : io_{io}, resolver_{io}, stream_{io, ssl_ctx_}, redirect_count_{redirect_count} {

    }

    void Run(const std::string & host,
             const std::string & port,
             const std::string & target,
             const std::string & filename,
             int version) {

        sys::error_code ec;
        file_parser_.body_limit(std::numeric_limits<std::uint64_t>::max());
        file_parser_.get().body().open(filename.c_str(), beast::file_mode::write, ec);
        if (ec) {
            std::cerr << "[ERROR]: couldn't open file" << std::endl;
            return;
        }

        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str())) {
            std::cerr << "[ERROR]: couldn't set SNI" << std::endl;
            return;
        }

        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req_.set(http::field::accept, "*/*");

        resolver_.async_resolve(host.c_str(),
                                port.c_str(),
                                std::bind(&Session::OnResolve, this->shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2)
                            );
    }
private:
    void OnResolve(const sys::error_code & ec, tcp::resolver::results_type results) {
        if (ec) {
            std::cerr << "[ERROR]: couldn't resolve host" << std::endl;
            return;
        }

        net::async_connect(
            stream_.lowest_layer(),
            results.begin(),
            results.end(),
            std::bind(&Session::OnConnect,
                      this->shared_from_this(),
                      std::placeholders::_1)
                );
    }

    void OnConnect(const sys::error_code & ec) {
        if (ec) {
            std::cerr << "[ERROR]: couldn't connect.." << std::endl;
            return;
        }

        stream_.async_handshake(
            ssl::stream_base::client,
            std::bind(&Session::OnHandshake,
                      this->shared_from_this(),
                      std::placeholders::_1)
        );
    }

    void OnHandshake(const sys::error_code & ec) {
        if (ec) {
            std::cerr << "[ERROR]: SSL Handshake failed" << std::endl;
            return;
        }

        http::async_write(
            stream_,
            req_,
            std::bind(&Session::OnWrite,
                      this->shared_from_this(),
                      std::placeholders::_1,
                      std::placeholders::_2)
        );
    }

    void OnWrite(const sys::error_code & ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "[ERROR]: couldn't write request" << std::endl;
            return;
        }
        
        http::async_read_header(
            stream_,
            buffer_,
            file_parser_,
            std::bind(&Session::OnHeaderRead,
                      this->shared_from_this(),
                      std::placeholders::_1,
                      std::placeholders::_2)
        );
    }

    void OnHeaderRead(const sys::error_code & ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "[ERROR]: couldn't read header" << std::endl;
            return;
        }

        const auto & head_res = file_parser_.get();
        http::status status = head_res.result();

        switch (status) {
            case http::status::moved_permanently:
            case http::status::found:
            case http::status::see_other:
            case http::status::temporary_redirect:
            case http::status::permanent_redirect:
                if (++redirect_count_ >= 5) {
                    std::cerr << "[ERROR]: too many redirects" << std::endl;
                } else {
                    file_parser_.get().body().close();

                    auto location = head_res[http::field::location];

                    if (location.empty()) {
                        std::cerr << "[ERROR]: redirect status but no Location header" << std::endl;
                    } else {
                        url_view u = parse_uri(location).value();

                        std::string new_host{u.host()};
                        std::string new_port{u.has_port() ? std::string(u.port()) : "443"};
                        std::string new_target{u.encoded_path()};
                        if (u.has_query()) {
                            new_target += "?";
                            new_target += std::string{u.encoded_query()};
                        }

                        std::make_shared<Session>(io_, redirect_count_)->Run(
                            new_host,
                            new_port,
                            new_target,
                            "output.html",
                            req_.version()
                        );
                    }

                }

                stream_.async_shutdown(
                    std::bind(&Session::OnShutdown,
                                this->shared_from_this(),
                                std::placeholders::_1)
                );

                return;

                break;
        }

        http::async_read(
            stream_,
            buffer_,
            file_parser_,
            std::bind(&Session::OnBodyRead,
                      this->shared_from_this(),
                      std::placeholders::_1,
                      std::placeholders::_2)
        );
    }

    void OnBodyRead(const sys::error_code & ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "[ERROR]: couldn't read body" << std::endl;
            return;
        }

        file_parser_.get().body().close();

        stream_.async_shutdown(
            std::bind(&Session::OnShutdown,
                      this->shared_from_this(),
                      std::placeholders::_1)
        );
    }

    void OnShutdown(const sys::error_code & ec) {
        if (ec) {
            // Shutdown errors are usually non-fatal here.
            std::cerr << "[WARN]: shutdown failed" << std::endl;
        }
        // Session can now be destroyed; if you need to notify other parts
        // of the application, do so here.
    }

    net::io_context & io_;
    ssl::context ssl_ctx_{ ssl::context::sslv23_client };
    net::ssl::stream<tcp::socket> stream_;
    tcp::resolver resolver_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    http::response_parser<http::file_body> file_parser_;
    beast::flat_buffer buffer_;
    int redirect_count_ = 0;
};

}