#pragma once

#include <iostream>
#include <istream>
#include <ostream>

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/url.hpp>
#include <boost/bind.hpp>

namespace http_client {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace ssl = net::ssl;
namespace urls = boost::urls;
namespace sys = boost::system;

using net::ip::tcp;

class HttpClient {
public:
	HttpClient(net::io_context& io, ssl::context& ctx, boost::urls::url& url) : url_{ url }, resolver_{ io }, ssl_stream_{ io, ctx } {
		std::ostream request_stream{&request_};
		request_stream << "GET " << url.path() << " HTTP/1.0\r\n";
		request_stream << "Host: " << url.host() << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";

		resolver_.async_resolve(url_.host(), url_.scheme(), boost::bind(&HttpClient::HandleResolve, this, net::placeholders::error, net::placeholders::results));
	}

	void HandleResolve(const sys::error_code & ec, tcp::resolver::results_type results) {
		if (!ec) {
			ssl_stream_.set_verify_mode(ssl::verify_peer);
			
			net::async_connect(ssl_stream_.lowest_layer(), results.begin(), results.end(), boost::bind(&HttpClient::HandleConnect, this, net::placeholders::error));
		} else {
			std::cerr << ec.what() << std::endl;
		}
	}

	void HandleConnect(const sys::error_code & ec) {
		if (!ec) {
			ssl_stream_.async_handshake(ssl::stream_base::client, boost::bind(&HttpClient::HandleHandshake, this, net::placeholders::error));
		} else {
			std::cerr << ec.what() << std::endl;
		}
	}

	void HandleHandshake(const sys::error_code & ec) {
		if (!ec) {
			net::async_write(ssl_stream_, request_, boost::bind(&HttpClient::HandleWriteRequest, this, net::placeholders::error));
		} else {
			std::cerr << ec.what() << std::endl;
		}
	}

	void HandleWriteRequest(const sys::error_code & ec) {
		if (!ec) {
			net::async_read(ssl_stream_, response_, boost::bind(&HttpClient::HandleReadResponse, this, net::placeholders::error));
		} else {
			std::cerr << ec.what() << std::endl;
		}
	}

	void HandleReadResponse(const sys::error_code & ec) {
		if (!ec) {
			std::cout << &response_;

			net::async_read(ssl_stream_, response_, boost::bind(&HttpClient::HandleReadResponse, this, net::placeholders::error));
		} else {
			std::cerr << ec.what() << std::endl;
		}
	}
private:
	boost::urls::url url_;
	tcp::resolver resolver_;
	ssl::stream<tcp::socket> ssl_stream_;
	net::streambuf request_;
	net::streambuf response_;
};

} // namespace http_client