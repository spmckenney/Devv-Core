import time
import zmq

def producer():
    context = zmq.Context()
    zmq_socket = context.socket(zmq.PUSH)
    zmq_socket.bind("tcp://*:5557")
    # Start your result manager and workers before you start your producers
    for num in range(20000):
        work_message = { 'num' : num }
        zmq_socket.send_json(work_message, flags=zmq.NOBLOCK)
        print("sent: "+str(work_message))
        time.sleep(1)

producer()
