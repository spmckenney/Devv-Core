//
//  Pubsub envelope publisher
//  Note that the zhelpers.h file also provides s_sendmore
//
// Olivier Chamoux <olivier.chamoux@fr.thalesgroup.com>

#include <iostream>
#include "io/zhelpers.hpp"

int main () {
    //  Prepare our context and publisher
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);
    std::cout << "Binding" << std::endl;
    publisher.bind("tcp://*:55556");

    std::cout << "Sleeping" << std::endl;
    sleep(10);
    
    while (1) {
      try {
        //  Write two messages, each with an envelope and content
        s_sendmore (publisher, "A");
        s_send (publisher, "We don't want to see this");
        s_sendmore (publisher, "B");
        s_send (publisher, "We would like to see this");
      } catch (zmq::error_t& e) {
        std::cerr << e.what() << std::endl;
      }
      sleep (1);

    }
    return 0;
}
