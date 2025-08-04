#pragma once

#ifndef BOOST_BIND_GLOBAL_PLACEHOLDERS
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#endif

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
		request_stream << "GET " << url.path() << " HTTP/1.1\r\n";
		request_stream << "Host: " << url.host() << "\r\n";
		request_stream << "User-Agent: Boost" << "\r\n";
		request_stream << "Content-Type: text/html\r\n";
		request_stream << "Connection: close\r\n\r\n";

		std::cout << "resolve host..." << std::endl;

		resolver_.async_resolve(url_.host(), "https", boost::bind(&HttpClient::HandleResolve, this, net::placeholders::error, net::placeholders::results));
	}

	void HandleResolve(const sys::error_code & ec, tcp::resolver::results_type results) {
		if (!ec) {
			std::cout << "Resolved!" << std::endl;

			net::async_connect(ssl_stream_.lowest_layer(), results.begin(), results.end(), boost::bind(&HttpClient::HandleConnect, this, net::placeholders::error));
		} else {
			std::cerr << "Resolving failed: " << ec.what() << std::endl;
		}
	}

	void HandleConnect(const sys::error_code & ec) {
		if (!ec) {
			std::cout << "Connected!" << std::endl;

			SSL_set_tlsext_host_name(ssl_stream_.native_handle(), url_.host().c_str());

			ssl_stream_.set_verify_mode(ssl::verify_peer);
			ssl_stream_.set_verify_callback(boost::bind(&HttpClient::VerifyCertificate, this, _1, _2));

			ssl_stream_.async_handshake(ssl::stream_base::client, boost::bind(&HttpClient::HandleHandshake, this, net::placeholders::error));
		} else {
			std::cerr << "Connection failed: " << ec.what() << std::endl;
		}
	}

	void HandleHandshake(const sys::error_code & ec) {
		if (!ec) {
			std::cout << "Handshake pass successfuly" << std::endl;
			std::cout << "Send request" << std::endl;

			net::async_write(ssl_stream_, request_, boost::bind(&HttpClient::HandleWriteRequest, this, net::placeholders::error));
		} else {
			std::cerr << "Handshake failed: " << ec.what() << std::endl;
		}
	}

	void HandleWriteRequest(const sys::error_code & ec) {
		if (!ec) {
			std::cout << "Request sent successfuly" << std::endl;
			std::cout << "Receive response" << std::endl;

			net::async_read(ssl_stream_, response_, boost::bind(&HttpClient::HandleReadResponse, this, net::placeholders::error, net::placeholders::bytes_transferred));
		} else {
			std::cerr << "Request failed:  " << ec.what() << std::endl;
		}
	}

	void HandleReadResponse(const sys::error_code & ec, std::size_t bytes_transferred) {
		std::cout << "Received bytes: " << bytes_transferred << std::endl;

		if (ec && ec != net::error::eof) {
			std::cerr << "Response failed: " << ec.what() << std::endl;
		} else {
			std::cout << &response_;

			
			if (ec != net::error::eof) {
				net::async_read(ssl_stream_, response_, boost::bind(&HttpClient::HandleReadResponse, this, net::placeholders::error, net::placeholders::bytes_transferred));
			} else {
				std::cout << "Reach end of response" << std::endl;
			}
		}
	}

	bool VerifyCertificate(bool preverified, ssl::verify_context & ctx) {
		std::cout << "verify_certificate (preverified: " << preverified << ")" << std::endl;

		char subject_name[256];
		X509 * cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        std::cout << "Verifying " << subject_name << "\n";
		
		return preverified;
	}
private:
	boost::urls::url url_;
	tcp::resolver resolver_;
	ssl::stream<tcp::socket> ssl_stream_;
	net::streambuf request_;
	net::streambuf response_;
};

} // namespace http_client