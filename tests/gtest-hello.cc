#include <functional>
#include <chrono>
#include <future>

#include <gtest/gtest.h>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/endpoint.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/message_buffer/message.hpp>

namespace {

class ws_client {
public:
    typedef std::function<void(std::string const&, bool)> msg_handler;

    virtual
    void init() = 0;

    virtual
    void connect(std::string const&) = 0;

    virtual
    void disconnect() = 0;

    virtual
    void on_msg_recv(std::string const &payload, bool binary = false) = 0;

    virtual
    void send_msg(std::string const &payload, bool binary = false) = 0;

    virtual
    bool is_secure() const = 0;

    virtual
    void set_msg_handler(msg_handler) = 0;
protected:
private:
};

template <typename configuration>
class ws_client_base
    : public ws_client {
public:
    typedef configuration config_type;
    typedef websocketpp::client<config_type> client_type;
    typedef typename client_type::message_ptr message_ptr;
    typedef typename client_type::connection_ptr connection_ptr;
    typedef websocketpp::lib::error_code error_code;
    typedef websocketpp::connection_hdl connection_hdl;

    ws_client_base() {
    }

    virtual
    ~ws_client_base() {
        this->dispose();
    }

    virtual
    void dispose() {
        disconnect();
    }

    virtual
    void init() override {
        using websocketpp::lib::bind;
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;

        _M_endpoint.set_access_channels(websocketpp::log::alevel::none);
        _M_endpoint.set_error_channels(websocketpp::log::elevel::none);

        _M_endpoint.init_asio();
        _M_endpoint.set_open_handler(
                bind(&ws_client_base::on_opened, this, _1));
        _M_endpoint.set_close_handler(
                bind(&ws_client_base::on_closed, this, _1));
        _M_endpoint.set_fail_handler(
                bind(&ws_client_base::on_failed, this, _1));
        _M_endpoint.set_message_handler(
                bind(&ws_client_base::on_message, this, _1, _2));
    }

    virtual
    void connect(std::string const &uri) override {
        using websocketpp::lib::bind;
        using websocketpp::lib::placeholders::_1;
        error_code ecode;
        connection_ptr conn = _M_endpoint.get_connection(uri, ecode);
        if (ecode)
            throw std::system_error(ecode);
        // actual connecting
        _M_endpoint.connect(conn);
        // move assignment
        _M_runner = std::thread(
                bind(&ws_client_base::blocking_running, this));
        // move assignment
        _M_connected_promise = std::promise<bool>();
        // wait for connected or connect failed
        if (!_M_connected_promise.get_future().get())
            throw std::runtime_error("connect failed!");
        _M_conn = conn;
    }

    virtual
    void disconnect() override {
        namespace status = websocketpp::close::status;
        _M_received_stop_signal = true;
        if (_M_conn) {
            // FIXME which one is better?
            // _M_endpoint.close(
            //         _M_conn,
            //         status::normal,
            //         "Normal Closure");
            _M_conn->close(status::normal, "Normal Closure");
            _M_conn = nullptr;
        }
        if (_M_runner.joinable())
            _M_runner.join();
    }

    virtual
    void on_opened(websocketpp::connection_hdl hdl) {
        _M_connected_promise.set_value(true);
    }

    virtual
    void on_failed(websocketpp::connection_hdl hdl) {
        try {
            throw std::system_error(
                    _M_endpoint.get_con_from_hdl(hdl)->get_ec());
        } catch (...) {
            _M_connected_promise.set_exception(std::current_exception());
        }
    }

    virtual
    void on_closed(websocketpp::connection_hdl hdl) {
    }

    virtual
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
        namespace opcode = websocketpp::frame::opcode;
        switch (msg->get_opcode()) {
            case opcode::TEXT:
                this->on_msg_recv(msg->get_payload(), false);
                break;
            case opcode::BINARY:
                this->on_msg_recv(msg->get_payload(), true);
                break;
            case opcode::CLOSE:
                break;
            case opcode::CONTROL_RSVE:
            case opcode::CONTROL_RSVB:
            case opcode::CONTROL_RSVC:
            case opcode::CONTROL_RSVD:
                break;
            default: break;
        }
    }

    virtual
    void send(std::string const &text, bool binary = false) {
        namespace opcode = websocketpp::frame::opcode;
        _M_endpoint.send(
                _M_conn->get_handle(),
                text,
                binary ? opcode::binary : opcode::text);
    }

    virtual
    void on_msg_recv(std::string const &payload, bool binary) override {
        if (!!_M_msg_handler)
            _M_msg_handler(payload, binary);
    }

    virtual
    void send_msg(const std::string &payload, bool binary) override {
        this->send(payload, binary);
    }

    // provide event loop for asio
    virtual
    void blocking_running() {
        _M_received_stop_signal = false;
        try {
            this->_M_endpoint.run();
        } catch (...) {
            if (!_M_received_stop_signal)
                _M_connected_promise.set_exception(std::current_exception());
        }
    }

    virtual
    bool is_secure() const override {
        return false;
    }

    virtual
    void set_msg_handler(msg_handler h) override {
        _M_msg_handler = h;
    }
protected:
    client_type _M_endpoint;
    std::thread _M_runner;
    connection_ptr _M_conn;
    volatile bool _M_received_stop_signal;
    std::promise<bool> _M_connected_promise;
    msg_handler _M_msg_handler;
private:
    ws_client_base(ws_client_base const&);
    ws_client_base& operator=(ws_client_base const&);
};

class ws_client_impl_raw
    : public ws_client_base<websocketpp::config::asio> {
};

class ws_client_impl_tls
    : public ws_client_base<websocketpp::config::asio_tls> {
public:
    typedef websocketpp::lib::asio::ssl::context context;
    typedef websocketpp::lib::shared_ptr<context> context_ptr;
    typedef ws_client_base<config_type> super;

    virtual
    void init() override {
        using websocketpp::lib::bind;
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;

        super::init();
        _M_endpoint.set_tls_init_handler(
                bind(&ws_client_impl_tls::on_tls_init, this, _1));
    }

    virtual
    context_ptr on_tls_init(connection_hdl hdl) {
        using websocketpp::lib::bind;
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;
        connection_ptr conn = _M_endpoint.get_con_from_hdl(hdl);
        context::method method = context::method::sslv23_client;
        context::options opts = 0;
        opts |= context::default_workarounds;
        opts |= context::no_sslv2;
        opts |= context::no_sslv3;
        boost::asio::ssl::verify_mode mode = boost::asio::ssl::verify_none;
        context_ptr ctx(new context(method));
        ctx->set_options(opts);
        ctx->set_verify_mode(mode);
        ctx->set_verify_callback(bind(&ws_client_impl_tls::on_tls_verify,
                    this, _1, _2));
        return ctx;
    }

    bool on_tls_verify(bool preverified,
            boost::asio::ssl::verify_context &ctx) {
        return true;
    }

    virtual
    bool is_secure() const override {
        return true;
    }
};

class websocket_client {
public:
    websocket_client() {
    }

    virtual
    ~websocket_client() {
    }

    void connect(std::string const &uri_string) {
        websocketpp::uri uri(uri_string);
        if (!uri.get_secure())
            _M_client.reset(new ws_client_impl_raw());
        else
            _M_client.reset(new ws_client_impl_tls());
        _M_client->init();
        _M_client->set_msg_handler(
                std::bind(
                    &websocket_client::handle_message,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2));
        _M_client->connect(uri_string);
    }

    void disconnect() {
        if (!!_M_client) {
            _M_client->disconnect();
            _M_client.reset();
        }
    }

    void handle_message(std::string const &payload, bool binary = false) {
        std::cout << "payload = " << payload << std::endl;
    }

    void send_msg(std::string const &payload, bool binary = false) {
        if (!_M_client)
            return;
        _M_client->send_msg(payload, binary);
    }
protected:
    std::shared_ptr<ws_client> _M_client;
private:
};

} // namespace anonymous

TEST(gtest_hello, test_websocket_client) {
    websocket_client client;
    for (int k = 0, m = 3; k < m; ++k) {
        client.connect("wss://127.0.0.1:9000");
        for (int i = 0, n = 2; i < n; ++i) {
            client.send_msg("Mr. White", false);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        client.disconnect();
    }
}
