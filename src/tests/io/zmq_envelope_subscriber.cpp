//
//  Pubsub envelope subscriber
//
// Olivier Chamoux <olivier.chamoux@fr.thalesgroup.com>

#include "io/zhelpers.hpp"

int main () {
    //  Prepare our context and subscriber
    zmq::context_t context(1);
    zmq::socket_t subscriber (context, ZMQ_SUB);
    std::cout << "Connecting" << std::endl;
    subscriber.connect("tcp://localhost:55556");
    std::cout << "sockopt" << std::endl;
    subscriber.setsockopt( ZMQ_SUBSCRIBE, "B", 1);
    std::cout << "done" << std::endl;

    while (1) {

        //  Read envelope with address
    std::cout << "recv env" << std::endl;
        std::string address = s_recv (subscriber);
        //  Read message contents
    std::cout << "recv contents" << std::endl;
        std::string contents = s_recv (subscriber);
        
        std::cout << "[" << address << "] " << contents << std::endl;
    }
    return 0;
}
