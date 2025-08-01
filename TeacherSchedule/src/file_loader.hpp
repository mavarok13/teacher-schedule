#pragma once

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = net::ssl;
using net::ip::tcp;

namespace file_loader {
	std::string GetSiteContent() {
		// ############## FIX THIS ##############
		std::string host = "www.rguk.ru";
		std::string port = "443";
		std::string target = "/students/schedule";
		int version = 11;
		// ######################################

		net::io_context io;

		ssl::context ssl_context{ssl::context::tls_client};
		ssl_context.set_verify_mode(
			ssl::context::verify_peer |
			ssl::context::verify_fail_if_no_peer_cert
		);
		ssl_context.set_default_verify_paths();

		tcp::resolver resolver{io};
		beast::tcp_stream tcp_stream{io};

		const auto results = resolver.resolve(host, port);

		tcp_stream.connect(results);

		http::request<http::string_body> req{http::verb::get, target, version};
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		http::write(tcp_stream, req);

		beast::flat_buffer buff;

		http::response<http::dynamic_body> res;

		http::read(tcp_stream, buff, res);

		std::cout << res << std::endl;

		beast::error_code ec;
		tcp_stream.socket().shutdown(tcp::socket::shutdown_both, ec);

		if (ec && ec != beast::errc::not_connected) {
			throw beast::system_error{ec};
		}

		std::cout << res << std::endl;
	}
} // namespace file_loader