#include <functional>
#include <chrono>
#include <future>

#include <gtest/gtest.h>

#include <websocketpp/config/debug_asio.hpp>
#include <websocketpp/endpoint.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/message_buffer/message.hpp>

namespace {

class ws_client {
public:
    typedef websocketpp::client<websocketpp::config::debug_asio> client_type;
    typedef client_type::message_ptr message_ptr;
    typedef client_type::connection_ptr connection_ptr;
    typedef websocketpp::lib::error_code error_code;
    typedef websocketpp::connection_hdl connection_hdl;

    ws_client() {
    }

    virtual
    ~ws_client() {
        dispose();
    }

    void dispose() {
        disconnect();
        if (_M_runner.joinable())
            _M_runner.join();
    }

    void init() {
        using websocketpp::lib::bind;
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;

        _M_endpoint.set_access_channels(websocketpp::log::alevel::all);
        _M_endpoint.set_error_channels(websocketpp::log::elevel::all);

        _M_endpoint.init_asio();
        _M_endpoint.set_open_handler(bind(&ws_client::on_opened, this, _1));
        _M_endpoint.set_close_handler(bind(&ws_client::on_closed, this, _1));
        _M_endpoint.set_fail_handler(bind(&ws_client::on_failed, this, _1));
        _M_endpoint.set_message_handler(
                bind(&ws_client::on_message, this, _1, _2));
    }

    void connect(std::string const &uri) {
        using websocketpp::lib::bind;
        using websocketpp::lib::placeholders::_1;
        error_code ecode;
        connection_ptr conn = _M_endpoint.get_connection(uri, ecode);
        if (ecode)
            throw std::system_error(ecode);
        // actual connecting
        _M_endpoint.connect(conn);
        // move assignment
        _M_runner = std::thread(bind(&ws_client::blocking_running, this));
        // move assignment
        _M_connected_promise = std::promise<bool>();
        // wait for connected or connect failed
        if (_M_connected_promise.get_future().get())
            _M_conn = conn;
    }

    void send(std::string const &text) {
        namespace opcode = websocketpp::frame::opcode;
        _M_endpoint.send(_M_conn->get_handle(), text, opcode::text);
    }

    void disconnect() {
        _M_received_stop_signal = true;
        if (_M_conn) {
            _M_conn->close(websocketpp::close::status::normal, "OK");
            _M_conn = nullptr;
        }
    }

    virtual
    void on_opened(websocketpp::connection_hdl hdl) {
        std::cout << "on_opened" << std::endl;
        _M_connected_promise.set_value(true);
    }

    virtual
    void on_failed(websocketpp::connection_hdl hdl) {
        std::cout << "on_failed" << std::endl;
    }

    virtual
    void on_closed(websocketpp::connection_hdl hdl) {
        std::cout << "on_closed" << std::endl;
    }

    virtual
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
        namespace opcode = websocketpp::frame::opcode;
        std::ostringstream oss;
        oss << "on_message ";
        switch (msg->get_opcode()) {
            case opcode::TEXT:
                oss << "[TEXT  ] " << msg->get_payload() << std::endl;
                break;
            case opcode::BINARY:
                oss << "[BINARY] " << msg->get_raw_payload() << std::endl;
                break;
            case opcode::CLOSE:
                oss << "[CLOSE ] " << msg->get_payload() << std::endl;
                break;
            case opcode::CONTROL_RSVE:
            case opcode::CONTROL_RSVB:
            case opcode::CONTROL_RSVC:
            case opcode::CONTROL_RSVD:
                break;
            default: break;
        }
        std::cout << oss.str() << std::flush;
    }

    // provide event loop for asio
    virtual
    void blocking_running() {
        try {
            this->_M_endpoint.run();
        } catch (...) {
            if (!_M_received_stop_signal)
                _M_connected_promise.set_exception(std::current_exception());
        }
    }
protected:
private:
    client_type _M_endpoint;
    std::thread _M_runner;
    connection_ptr _M_conn;
    volatile bool _M_received_stop_signal;
    std::promise<bool> _M_connected_promise;
};

} // namespace anonymous

TEST(gtest_hello, test1) {
    ws_client client;
    client.init();
    client.connect("ws://127.0.0.1:9000");
    client.send("Mr. White");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
