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

class HttpClientBase {
public:
//	* Get thiss class ptr virtual method
	virtual std::shared_ptr<HttpClientBase> GetPtr() = 0;

//	* Constuctor
	HttpClientBase(net::io_context & io, const urls::url & url, ssl::context & ctx)
	: resolver_{io}, url_{url}, ssl_stream_{io, ctx} {
		std::string host = url.host().c_str();
		
		if (!SSL_set_tlsext_host_name(ssl_stream_.native_handle(), host.c_str())) {
			sys::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
			throw sys::system_error{ec};
		}
	}

protected:
	virtual void Write() = 0;
	virtual void Read() = 0;
	virtual void HandleRequest() = 0;

//	* Resolve handler method
	void OnResolve(const sys::error_code & ec, tcp::resolver::results_type results) {
		if (!ec) {
			net::async_connect(	ssl_stream_.lowest_layer(),
								results.begin(),
								results.end(),
								[self = GetPtr()] (const sys::error_code & ec, auto endpoints) {
									self->OnConnect(ec);
								});
		} else {
			throw std::runtime_error(ec.what());
		}
 	}

//	* Connect handler method
	void OnConnect(const sys::error_code & ec) {
		if (!ec) {
			ssl_stream_.set_verify_mode(ssl::verify_peer);
			ssl_stream_.set_verify_callback([self = GetPtr()] (bool preverified, ssl::verify_context & ctx) {
				return self->PreverifyCertificate(preverified, ctx);
			});
			
			ssl_stream_.async_handshake(ssl::stream_base::client, [self = GetPtr()] (const sys::error_code & ec) {
				self->OnHandshake(ec);
			});
		} else {
			throw std::runtime_error(ec.what());
		}
	}

//	* Handshake handler method
	void OnHandshake(const sys::error_code & ec) {
		if (!ec) {
			Write();
		} else {
			throw std::runtime_error(ec.what());
		}
	}

//	* Write handler method
	void OnWrite(const sys::error_code & ec, std::size_t transferred_bytes) {
		if (!ec) {
			Read();
		} else {
			throw std::runtime_error(ec.what());
		}
	}

//	* Read handler method
	void OnRead(const sys::error_code & ec, std::size_t transferred_bytes) {
		if (!ec) {
			HandleRequest();
		} else {
			throw std::runtime_error(ec.what());
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
	
	tcp::resolver resolver_;
	urls::url url_;
	ssl::stream<tcp::socket> ssl_stream_;
};

template <typename Request, typename Response, typename Handler>
class HttpClient : public HttpClientBase, public std::enable_shared_from_this<HttpClient<Request, Response, Handler>> {

public:
//	* Create
	static std::shared_ptr<HttpClient> Create(
		net::io_context & io,
		const urls::url & url,
		ssl::context & ctx,
		Request req,
		Response res,
		Handler handler
	) {
		
		return std::make_shared<HttpClient<Request, Response, Handler>>(io, url, ctx, std::move(req), std::move(res), std::move(handler));

	}
	
//	* Constuctor
	HttpClient(net::io_context & io, const urls::url & url, ssl::context & ctx, Request req, Response res, Handler handler)
	: HttpClientBase{io, url, ctx}, req_{std::move(req)}, res_{std::move(res)}, handler_{std::move(handler)} {}

//	* Get this class ptr method
	std::shared_ptr<HttpClientBase> GetPtr() override {
		return this->shared_from_this();
	}
	
	void SendRequest() {
		resolver_.async_resolve(url_.host(), "https", [self = this->shared_from_this()] (const sys::error_code & ec, tcp::resolver::results_type results) {
			self->OnResolve(ec, results);
		});
	}

protected:
	void Write() override {
		http::async_write(ssl_stream_, req_, [self = this->shared_from_this()] (const sys::error_code & ec, size_t bytes_transferred) {
			self->OnWrite(ec, bytes_transferred);
		});
	}

	void Read() override {
		http::async_read(ssl_stream_, read_buff_, res_, [self = this->shared_from_this()] (const sys::error_code & ec, size_t bytes_transferred) {
			self->OnRead(ec, bytes_transferred);
		});
	}

	void HandleRequest() override {
		handler_(std::move(res_));
	}
private:
	Request req_;
	Response res_;
	Handler handler_;
	beast::flat_buffer read_buff_;
};

} // namespace http_client