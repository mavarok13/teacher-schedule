#pragma once

#include <iostream>
#include <memory>
#include <limits>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <cstdlib>

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <openssl/ssl.h>
#include <openssl/x509.h>

namespace net = boost::asio;
namespace sys = boost::system;
namespace beast = boost::beast;
namespace ssl = net::ssl;
namespace http = beast::http;
using tcp = net::ip::tcp;

namespace http_client {

class Session : public std::enable_shared_from_this<Session> {
public:
    Session (net::io_context & io) : io_{io}, stream_{io, ssl_ctx_}, resolver_{io} {
        ssl_ctx_.set_default_verify_paths();

        // Try to load CA bundle from environment variables if provided.
        if (const char * cafile = std::getenv("SSL_CERT_FILE")) {
            try {
                ssl_ctx_.load_verify_file(cafile);
                std::cerr << "[INFO]: loaded CA file from SSL_CERT_FILE: " << cafile << std::endl;
            } catch (const std::exception & e) {
                std::cerr << "[WARN]: failed to load SSL_CERT_FILE '" << cafile << "': " << e.what() << std::endl;
            }
        } else if (const char * cafile2 = std::getenv("CURL_CA_BUNDLE")) {
            try {
                ssl_ctx_.load_verify_file(cafile2);
                std::cerr << "[INFO]: loaded CA file from CURL_CA_BUNDLE: " << cafile2 << std::endl;
            } catch (const std::exception & e) {
                std::cerr << "[WARN]: failed to load CURL_CA_BUNDLE '" << cafile2 << "': " << e.what() << std::endl;
            }
        }

        stream_.set_verify_mode(ssl::verify_peer);

    }

    Session (net::io_context & io, int redirect_count)
        : io_{io}, stream_{io, ssl_ctx_}, resolver_{io}, redirects_count_{redirect_count} {
        ssl_ctx_.set_default_verify_paths();

        if (const char * cafile = std::getenv("SSL_CERT_FILE")) {
            try {
                ssl_ctx_.load_verify_file(cafile);
                std::cerr << "[INFO]: loaded CA file from SSL_CERT_FILE: " << cafile << std::endl;
            } catch (const std::exception & e) {
                std::cerr << "[WARN]: failed to load SSL_CERT_FILE '" << cafile << "': " << e.what() << std::endl;
            }
        } else if (const char * cafile2 = std::getenv("CURL_CA_BUNDLE")) {
            try {
                ssl_ctx_.load_verify_file(cafile2);
                std::cerr << "[INFO]: loaded CA file from CURL_CA_BUNDLE: " << cafile2 << std::endl;
            } catch (const std::exception & e) {
                std::cerr << "[WARN]: failed to load CURL_CA_BUNDLE '" << cafile2 << "': " << e.what() << std::endl;
            }
        }

        stream_.set_verify_mode(ssl::verify_peer);

    }

    void Run(const std::string & host,
             const std::string & port,
             const std::string & target,
             const std::string & filename,
             int version) {

        filename_ = filename;
        host_ = host;
        port_ = port;
        target_ = target;
        version_ = version;

        sys::error_code ec;
        file_parser_.body_limit(std::numeric_limits<std::uint64_t>::max());
        file_parser_.get().body().open(filename.c_str(), beast::file_mode::write, ec);
        if (ec) {
            std::cerr << "[ERROR]: couldn't open file: " << ec.message() << std::endl;
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

        // set verify callback so we can inspect and optionally override verification
        stream_.set_verify_callback(std::bind(&Session::verify_certificate, this, std::placeholders::_1, std::placeholders::_2));

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
            std::cerr << "[ERROR]: couldn't resolve host: " << ec.message() << std::endl;
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
            std::cerr << "[ERROR]: couldn't connect: " << ec.message() << std::endl;
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
            std::cerr << "[ERROR]: SSL Handshake failed: " << ec.message() << std::endl;
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
            std::cerr << "[ERROR]: couldn't write request: " << ec.message() << std::endl;
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
            std::cerr << "[ERROR]: couldn't read header: " << ec.message() << std::endl;
            return;
        }

        const auto & head_res = file_parser_.get();
        http::status status = head_res.result();

        // Diagnostic output: status and common headers
        std::cout << "[DEBUG]: HTTP status: " << static_cast<int>(status) << "\n";
        auto cl = head_res[http::field::content_length];
        if (!cl.empty()) std::cout << "[DEBUG]: Content-Length: " << cl << "\n";
        auto te = head_res[http::field::transfer_encoding];
        if (!te.empty()) std::cout << "[DEBUG]: Transfer-Encoding: " << te << "\n";
        auto loc = head_res[http::field::location];
        if (!loc.empty()) std::cout << "[DEBUG]: Location: " << loc << "\n";

        switch (status) {
            case http::status::moved_permanently:
            case http::status::found:
            case http::status::see_other:
            case http::status::temporary_redirect:
            case http::status::permanent_redirect:
                if (++redirects_count_ >= 5) {
                    std::cerr << "[ERROR]: too many redirects" << std::endl;
                } else {
                    file_parser_.get().body().close();

                    auto location = head_res[http::field::location];

                    if (location.empty()) {
                        std::cerr << "[ERROR]: redirect status but no Location header" << std::endl;
                    } else {
                        auto r = boost::urls::parse_uri_reference(location);
                        if (!r) {
                            std::cerr << "[ERROR]: failed to parse Location header: " << location << std::endl;
                        } else {
                            auto u = r.value();

                            std::string new_host{u.host().empty() ? std::string(host_) : std::string(u.host())};
                            std::string new_port{u.has_port() ? std::string(u.port()) : std::string(port_)};

                            std::string new_target;
                            if (u.encoded_path().empty()) {
                                // relative reference or empty path: use the location string directly
                                new_target = std::string(location);
                            } else {
                                new_target = std::string{u.encoded_path()};
                                if (u.has_query()) {
                                    new_target += "?";
                                    new_target += std::string{u.encoded_query()};
                                }
                            }

                            std::make_shared<Session>(io_, redirects_count_)->Run(
                                new_host,
                                new_port,
                                new_target,
                                filename_,
                                req_.version()
                            );
                        }
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

        // Try to report downloaded file size
        try {
            auto sz = std::filesystem::file_size(filename_);
            std::cout << "[INFO]: downloaded " << sz << " bytes to " << filename_ << std::endl;
        } catch (const std::exception & e) {
            std::cout << "[WARN]: unable to get file size: " << e.what() << std::endl;
        }

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

    // Verification callback. Returns true when certificate is accepted.
    // If environment variable SSL_NO_VERIFY=1 is set, verification will be bypassed (for debugging only).
    bool verify_certificate(bool preverified, ssl::verify_context & ctx) {
        std::cerr << "[DEBUG]: certificate preverified=" << preverified << std::endl;
        if (preverified) return true;
        const char * env = std::getenv("SSL_NO_VERIFY");
        if (env && std::string(env) == "1") {
            std::cerr << "[WARN]: SSL verification disabled via SSL_NO_VERIFY=1" << std::endl;
            return true;
        }
        return false;
    }

    net::io_context & io_;
    ssl::context ssl_ctx_{ssl::context::sslv23_client};
    net::ssl::stream<tcp::socket> stream_;
    tcp::resolver resolver_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    http::response_parser<http::file_body> file_parser_;
    beast::flat_buffer buffer_;
    std::string filename_;
    int redirects_count_ = 0;
    std::string host_;
    std::string port_;
    std::string target_;
    int version_;
};

} // namespace http_client