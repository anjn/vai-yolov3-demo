#pragma once

namespace demo {

static void dynamic_batching(
  zmq::socket_t& frontend,
  zmq::socket_t& backend,
  int max_batch,
  int max_latency_ms
) {

  zmq::pollitem_t items[] = {
    { static_cast<void*>(frontend), 0, ZMQ_POLLIN, 0 },
    { static_cast<void*>(backend), 0, ZMQ_POLLIN, 0 }
  };

  while (1) {
    zmq::message_t message;
    int more;               //  Multipart detection

    zmq::poll(&items[0], 2, -1);

    if (items[0].revents & ZMQ_POLLIN) {
      while (1) {
        //  Process all parts of the message
        frontend.recv(&message);
        size_t more_size = sizeof (more);
        frontend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        backend.send(message, more? ZMQ_SNDMORE : 0);

        if (!more)
          break;      //  Last message part
      }
    }
    if (items[1].revents & ZMQ_POLLIN) {
      while (1) {
        //  Process all parts of the message
        backend.recv(&message);
        size_t more_size = sizeof (more);
        backend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        frontend.send(message, more? ZMQ_SNDMORE : 0);
        if (!more)
          break;      //  Last message part
      }
    }
  }
}

}
