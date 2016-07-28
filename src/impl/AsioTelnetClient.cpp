/* AsioTelnetClient.cpp
A simple demonstration telnet client with Boost asio

Parameters:
hostname or address
port - typically 23 for telnet service

To end the application, send Ctrl-C on standard input
*/

#include <iostream>

#include "AsioTelnetClient.h"

#include "TelnetProtocol.h"

#ifdef POSIX
#   include <termios.h>
#endif

using namespace std;


AsioTelnetClient::AsioTelnetClient(boost::asio::io_service& io_service, tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service), socket_(io_service)
{
    connect_start(endpoint_iterator);

    thread_ = std::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, &io_service));
}

AsioTelnetClient::~AsioTelnetClient()
{
    close();
}

void AsioTelnetClient::close() // call the do_close function via the io service in the other thread
{
    callback_receive_data_fn_ = nullptr;
    callback_closesocket_fn_ = nullptr;

    if (thread_)
    {
        io_service_.post(boost::bind(&AsioTelnetClient::do_close, this));

        thread_->join();
        thread_ = nullptr;
    }
}

void AsioTelnetClient::write(const char msg) // pass the write data to the do_write function via the io service in the other thread
{
    std::string buffer;

    buffer += msg;

    io_service_.post(boost::bind(&AsioTelnetClient::do_write, this, buffer));
}

void AsioTelnetClient::write(const std::string msg) // pass the write data to the do_write function via the io service in the other thread
{
    io_service_.post(boost::bind(&AsioTelnetClient::do_write, this, msg));
}

// asynchronously connect a socket to the specified remote endpoint and call connect_complete when it completes or fails
void AsioTelnetClient::connect_start(tcp::resolver::iterator endpoint_iterator)
{  
    tcp::endpoint endpoint = *endpoint_iterator;

    socket_.async_connect(endpoint,
        boost::bind(&AsioTelnetClient::connect_complete,
            this,
            boost::asio::placeholders::error,
            ++endpoint_iterator));
}

void AsioTelnetClient::connect_complete(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator)
{ 
    // the connection to the server has now completed or failed and returned an error
    if (!error) // success, so start waiting for read data
        read_start();
    else
    {
        if (endpoint_iterator != tcp::resolver::iterator())
        { // failed, so wait for another connection event
            socket_.close();
            connect_start(endpoint_iterator);
        }
    }
}

void AsioTelnetClient::read_start(void)
{ // Start an asynchronous read and call read_complete when it completes or fails
    socket_.async_read_some(boost::asio::buffer(read_msg_, max_read_length),
        boost::bind(&AsioTelnetClient::read_complete,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void AsioTelnetClient::read_complete(const boost::system::error_code& error, size_t bytes_transferred)
{ // the asynchronous read operation has now completed or failed and returned an error
    if (!error)
    { 
        // read completed, so process the data
        if (pending_buf_.length() > 0)
        {
            int offset = pending_buf_.length();

            for (int index = bytes_transferred - 1; index >= 0; index--)
                read_msg_[index + offset] = read_msg_[index];

            for (int index = 0; index < offset; index++)
                read_msg_[index] = pending_buf_[index];

            pending_buf_.clear();
        }

        for (int index = 0; index < bytes_transferred;)
        {
            if (read_msg_[index] == IAC)
            {
                if (bytes_transferred - index >= 3)
                {
                    unsigned char receiveData[3] = { read_msg_[index], read_msg_[index + 1], read_msg_[index + 2] };

                    handleCommand(receiveData);

                    index += 3;
                }
                else
                {
                    for (int sub_index = index; sub_index < bytes_transferred; sub_index++)
                        pending_buf_ += read_msg_[sub_index];

                    break;
                }
            }
            else if (read_msg_[index] == VT_ESC)
            {
                index += 2;
            }
            else
            {
                current_line_buffer += read_msg_[index];

                if (read_msg_[index] == '\n')
                    previous_received_line_buffer = current_line_buffer;

                index++;
            }
        }

        if (current_line_buffer.length() > 0 && callback_receive_data_fn_)
            callback_receive_data_fn_(current_line_buffer);

        current_line_buffer.clear();

        //  TRACE("\n%s", strBuf.c_str());                                     //cout << "\n";
        read_start(); // start waiting for another asynchronous read again
    }
    else
    {
        do_close();
    }
}

void AsioTelnetClient::do_write(const std::string msg)
{ // callback to handle write call from outside this class
    bool write_in_progress = !write_msgs_.empty(); // is there anything currently being written?

    for (auto it : msg)
        write_msgs_.push_back(it); // store in write buffer
    
    if (!write_in_progress) // if nothing is currently being written, then start
        write_start();
}

void AsioTelnetClient::write_start(void)
{ // Start an asynchronous write and call write_complete when it completes or fails
    boost::asio::async_write(socket_,
        boost::asio::buffer(&write_msgs_.front(), 1),
        boost::bind(&AsioTelnetClient::write_complete,
            this,
            boost::asio::placeholders::error));
}

void AsioTelnetClient::write_complete(const boost::system::error_code& error)
{ // the asynchronous read operation has now completed or failed and returned an error
    if (!error)
    { // write completed, so send next write data
        write_msgs_.pop_front(); // remove the completed data
        if (!write_msgs_.empty()) // if there is anthing left to be written
            write_start(); // then start sending the next item in the buffer
    }
    else
    {
        do_close();
    }
}

void AsioTelnetClient::do_close()
{
    socket_.close();

    if (callback_closesocket_fn_)
        callback_closesocket_fn_();
}

// Deal with a single command
int AsioTelnetClient::handleCommand(unsigned char* commandData)
{
    unsigned char currentByte = *(++commandData);
    commandData++;

    if (currentByte == DO || currentByte == DONT)
        respondToRequest(currentByte, *commandData);
    else if (currentByte == WILL || currentByte == WONT)
        respondToStatement(currentByte, *commandData);
    //else
    //	throw "Unknown code!";

    return 3;
}


// Respond to a DO/DONT request
void AsioTelnetClient::respondToRequest(unsigned char command, unsigned char option)
{
    std::string buf;

    buf += IAC;

    if (command == DONT || option == TERMINALTYPE || option == WINDOWSIZE || option == TERMINALSPEED || option == ENVIRONMENTOPTION || option == XDISPLAYLOCATION || option == ENVIRONMENTOPTION2)
        buf += WONT;
    else
        buf += WILL;

    buf += option;

    write(buf);
}

// Respond to a WILL/WONT statement
void AsioTelnetClient::respondToStatement(unsigned char command, unsigned char option)
{
    std::string buf;

    buf += IAC;

    if (command == WONT)
        buf += DONT;
    else
        buf += DO;

    buf += option;

    write(buf);
}
