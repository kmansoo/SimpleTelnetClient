# SimpleTelnetClient
This simple telnet client is based on boost c++ library. Also it uses Boost.Asio and lambda function. 

# How to build
##1. Windows
   SimpleTelnetClient has a solution file for Visual Studio 2015, so you can build very easy.

##2. Linux
###  Preparation
  You need to build and install the boost c++ library before you try to build this project.

###  Build Steps
```bash
mkdir build
cd build
cmake ..
make
```

# Example
```C++
#include <iostream>

#include "impl/AsioTelnetClient.h"

int main(int argc, char* argv[])
{
    std::string dest_ip;
    std::string dest_port;

    if (argc != 3)
    {
        std::cerr << "Usage: telnet <host> <port>\n";
        return 1;
    }
    else
    {
        dest_ip = argv[1];
        dest_port = argv[2];
    }

    try
    {
        std::cout << "SimpleTelnetClient is tring to connect " << dest_ip << ":" << dest_port << std::endl;

        boost::asio::io_service io_service;

        // resolve the host name and port number to an iterator that can be used to connect to the server
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(dest_ip, dest_port);
        tcp::resolver::iterator iterator = resolver.resolve(query);
        // define an instance of the main class of this program

        AsioTelnetClient telnet_client(io_service, iterator);

        // set a callback lambda function to process a message when it's received from telnet server.
        telnet_client.setReceivedSocketCallback([](const std::string& message) {
            std::cout << message;
        });

        // set a callback lambda function to realize an socket problem event.
        telnet_client.setClosedSocketCallback([]() {
            std::cout << " # disconnected" << std::endl;
        });

        while (1)
        {
            char ch;
            std::cin.get(ch); // blocking wait for standard input

            if (ch == 3) // ctrl-C to end program
                break;

            telnet_client.write(ch);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
```
