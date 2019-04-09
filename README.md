## RelayServer
This repository contains an implementation of a relay server in C. The relay server utilizes UDP sockets for swift but non-lossless
packet forwarding. In particular, this implementation was created to forward a VLC video stream from one computer to another. The
_packet_loss_ function is used to simulate packet loss over a network based on a specified loss rate. Modifying the loss rate results
in resolution changes to the output stream on the computer forwarded to.
