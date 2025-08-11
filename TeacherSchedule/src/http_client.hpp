#pragma once

#ifndef BOOST_BIND_GLOBAL_PLACEHOLDERS
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#endif

#include <iostream>
#include <istream>
#include <ostream>
#include <functional>
#include <memory>

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/url.hpp>
#include <boost/bind.hpp>

namespace http_client {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = net::ssl;
namespace urls = boost::urls;
namespace sys = boost::system;
using tcp = net::ip::tcp;

template <typename RequestBody, typename RequestFields, typename ResponseBody, typename ResponseFields>
class HttpClient : public std::enable_shared_from_this<HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>> {

using Request = http::request<RequestBody, RequestFields>;
using Response = http::response<ResponseBody, ResponseFields>;

using RequestHandle = std::function<void(Response)>;

public:
//	* Create
	static std::shared_ptr<HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>> Create(
		net::io_context & io,
		urls::url url,
		ssl::context & ctx,
		Request request,
		Response response,
		RequestHandle request_handler
	) {

		return std::make_shared<HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>>(io, url, ctx, std::move(request), std::move(response), request_handler);

	}

//	* Constuctor
	HttpClient(net::io_context & io, urls::url url, ssl::context & ctx, Request request, Response response, RequestHandle request_handler)
	: resolver_{io}, url_{url}, ssl_stream_{io, ctx}, request_{request}, response_{std::move(response)}, read_buff_{}, request_handler_{request_handler} {

		std::string host = url.host().c_str();

		if (!SSL_set_tlsext_host_name(ssl_stream_.native_handle(), host.c_str())) {
			sys::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
			throw sys::system_error{ec};
		}

	}
	
	std::shared_ptr<HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>> GetPtr() {
		return this->shared_from_this();
	}
	
	void SendRequest() {
		resolver_.async_resolve(url_.host(), "https", boost::bind(&HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>::HandleResolveAsync, GetPtr(), net::placeholders::error, net::placeholders::results));
	}
private:
//	* Resolve handler method
	void HandleResolveAsync(const sys::error_code & ec, tcp::resolver::results_type results) {
		if (!ec) {
			// net::async_connect(	ssl_stream_.lowest_layer(),
			// 					results.begin(),
			// 					results.end(),
			// 					boost::bind(&HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>::HandleConnectAsync, this->shared_from_this(), net::placeholders::error, net::placeholders::iterator));

			net::async_connect(	ssl_stream_.lowest_layer(),
								results.begin(),
								results.end(),
								boost::bind(&HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>::HandleConnectAsync, GetPtr(), net::placeholders::error));
		} else {
			std::cerr << "Resolving error: " << ec.what() << std::endl;
		}
 	}

//	* Connect handler method
	// void HandleConnectAsync(const sys::error_code & ec, std::vector<tcp::endpoint>::iterator iterator) {
	void HandleConnectAsync(const sys::error_code & ec) {
		if (!ec) {
			ssl_stream_.set_verify_mode(ssl::verify_peer);
			ssl_stream_.set_verify_callback(boost::bind(&HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>::PreverifyCertificate, GetPtr(), _1, _2));
			
			ssl_stream_.async_handshake(ssl::stream_base::client, boost::bind(&HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>::HandleHandshakeAsync, GetPtr(), net::placeholders::error));
		} else {
			std::cerr << "Connection error: " << ec.what() << std::endl;
		}
	}

//	* Pre-verify certificate callback method
	bool PreverifyCertificate(bool preverified, ssl::verify_context & ctx) {
		std::cout << "Preverify certificate status: " << (preverified ? "true" : "false") << std::endl;

		char subject_name[256];
		X509 * cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        std::cout << "Verifying " << subject_name << "\n";

		return preverified;
	}

//	* Handshake handler method
	void HandleHandshakeAsync(const sys::error_code & ec) {
		if (!ec) {
			http::async_write(ssl_stream_, request_, boost::bind(&HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>::HandleWriteAsync, GetPtr(), net::placeholders::error));
		} else {
			std::cerr << "Handshake error: " << ec.what() << std::endl;
		}
	}

//	* Write handler method
	void HandleWriteAsync(const sys::error_code & ec) {
		if (!ec) {
			http::async_read(ssl_stream_, read_buff_, response_, boost::bind(&HttpClient<RequestBody, RequestFields, ResponseBody, ResponseFields>::HandleReadAsync, GetPtr(), net::placeholders::error, net::placeholders::bytes_transferred));
		} else {
			std::cerr << "Write error:" << ec.what() << std::endl;
		}
	}

//	* Read handler method
	void HandleReadAsync(const sys::error_code & ec, std::size_t transferred_bytes) {
		if (!ec) {
			request_handler_(std::move(response_));
		} else {
			std::cerr << "Read error: " << ec.what() << std::endl;
		}
	}

	tcp::resolver resolver_;
	urls::url url_;
	ssl::stream<tcp::socket> ssl_stream_;
	Request request_;
	Response response_;
	beast::flat_buffer read_buff_;
	RequestHandle request_handler_;
};

} // namespace http_client