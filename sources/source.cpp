// Copyright 2021 Artamonov Mark <a.mark.2001@mail.ru>

#include <header.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/pthread/recursive_mutex.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>

using boost::asio::io_service;
static io_service the_service;
using boost::asio::ip::tcp;
boost::recursive_mutex the_mutex;

class talk_to_client {
public:
    talk_to_client()
            : the_time(std::chrono::
            system_clock::now()),
            the_socket(
                    the_service) {}
    tcp::socket& get_the_socket() {
        return the_socket;
    }

    std::string get_the_username() {
        return the_username;
    }

    void reset_the_username(
            std::string& an_username) {
        the_username = an_username;
    }

    void make_an_answer_to_the_client(
            const std::string& an_answer) {
        the_socket.write_some(boost::
        asio::buffer(an_answer));
    }

    bool check_if_the_time_is_out() {
        return (std::chrono::
        system_clock::now() -
        the_time).count() >= 5;
    }
private:
    std::chrono::system_clock::
    time_point the_time;
    std::string the_username;
    boost::asio::ip::tcp::
    socket the_socket;
};

unsigned short
the_port = 8001;
unsigned short
the_username_maxsize = 20;
unsigned short
the_command_line_maxsize = 1024;
typedef boost::shared_ptr
        <talk_to_client> the_ptr_of_client;
typedef std::vector
        <the_ptr_of_client>
        the_array_of_client_ptrs;
the_array_of_client_ptrs the_clients;

void accept_thread() {
    tcp::acceptor the_acceptor(
            the_service,
            tcp::endpoint(
                    tcp::v4(), the_port));
    while (true) {
        BOOST_LOG_TRIVIAL(info)
            << "Start \n The id of the thread is: "
            << std::this_thread::get_id()
            << "\n";
        std::string an_username;
        the_ptr_of_client the_new_one(
                new talk_to_client());
        the_acceptor.accept(
                the_new_one->
                get_the_socket());
        boost::recursive_mutex::
        scoped_lock the_lock(
                the_mutex);
        the_new_one->get_the_socket().read_some(
                boost::asio::buffer(an_username,
                        the_username_maxsize));
        if (an_username.empty()) {
            the_new_one->make_an_answer_to_the_client(
                    "Sorry, failed to login");
            BOOST_LOG_TRIVIAL(info)
            << "Login is failed \n The id of the thread is: \n"
            << std::this_thread::get_id() << "\n";
        } else {
            the_new_one->reset_the_username(
                    an_username);
            the_clients.push_back(
                    the_new_one);
            the_new_one->make_an_answer_to_the_client(
                    "Login is ok");
            BOOST_LOG_TRIVIAL(info)
                << "Login is ok \n The username is: "
                << an_username
                << "\n";
        }
    }
}

std::string get_the_full_list_of_clients() {
    std::string the_full_list_of_clients;
    for (auto& a_client
    : the_clients) the_full_list_of_clients +=
            a_client->get_the_username() + " ";
    return the_full_list_of_clients;
}

void handle_clients_thread() {
    while (true) {
        boost::this_thread::
        sleep(boost::posix_time::
        millisec(10));
        boost::recursive_mutex::
        scoped_lock the_lock(
                the_mutex);
        for (auto& a_client : the_clients) {
            std::string the_command_line;
            a_client->get_the_socket().read_some(
                    boost::asio::buffer(
                            the_command_line,
                            the_command_line_maxsize));
            if (the_command_line ==
            "client_list_chaned")
                a_client->make_an_answer_to_the_client(
                        get_the_full_list_of_clients());
            a_client->make_an_answer_to_the_client(
                    "The ping is ok");
            BOOST_LOG_TRIVIAL(info)
            << "Answer were made to the client \n The username is: "
            << a_client->get_the_username() << "\n";
        }
        the_clients.erase(std::remove_if(
                the_clients.begin(),
                the_clients.end(),
                boost::bind(
                        &talk_to_client::
                        check_if_the_time_is_out, _1)),
                          the_clients.end());
    }
}

int main() {
    boost::thread_group threads;
    threads.create_thread(accept_thread);
    threads.create_thread(handle_clients_thread);
    threads.join_all();
}