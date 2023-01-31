# ACM Simulation
This project is a study on reliable message passing with ACM (Adaptive Coding and Modulation).

The program simulates a message passing channel (transmitter - medium - receiver). Quadrature 
modulation is used to increase spectral efficiency (bit/s/Hz) of transmission, but it makes the 
signal more succeptible to noise; channel coding is used for forward error correction to increase 
reliability by applying redundancy. To resolve this, VCM (Variable Coding and Modulation) is used, 
providing different coding and modulation schemes depending on the circumstances. ACM is the 
evolution of VCM, utilizing the return channel to adjust transmission parameters in response to 
channel conditions.

To facilitate ACM, a signalling system on the physical layer of the protocol is necessary. A header 
of sorts can be read prior to demodulating and decoding of the frame payload, which will inform the 
receiver of the coding and modulation parameters in this frame.

This project is structured after the DVB-S2 protocol, according to ETSI EN 302 307-1. The 
transmitting part of the program has the following submodules:
1. Input interface;
2. Channel coding (FEC);
3. Bit mapping into signal constellations;
4. Framing on the physical layer - PLHEADER for detection and signalling of the ACM;
5. Quadrature modulation.

This project uses [AFF3CT](https://github.com/aff3ct/aff3ct) library to simulate the channel and collect frame error statistics.
