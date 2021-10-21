/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tcpsourcemeasuredelay.hpp"

#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern const int num_delay_slots;
extern struct timespec delay_slots[];
extern const double delay_interval;

using namespace Csdrx;

template<typename T>
TcpSourceMeasureDelay<T>::~TcpSourceMeasureDelay() {
    ::close(sock);
}

template <typename T>
TcpSourceMeasureDelay<T>::TcpSourceMeasureDelay(in_addr_t ip, unsigned short port) {
    sockaddr_in remote{};
    std::memset(&remote, 0, sizeof(remote));

    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = ip;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw Csdr::NetworkException("unable to create socket");
    }

    if (connect(sock, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
        throw Csdr::NetworkException("connection failed");
    }

}

template <typename T>
TcpSourceMeasureDelay<T>::TcpSourceMeasureDelay(unsigned short port) :
    TcpSourceMeasureDelay(inet_addr("127.0.0.1"), port)
{}

template <typename T>
void TcpSourceMeasureDelay<T>::setWriter(Csdr::Writer<T> *writer) {
    Csdr::Source<T>::setWriter(writer);
    if (thread == nullptr) {
        thread = new std::thread( [this] () { loop(); });
    }
}

template <typename T>
void TcpSourceMeasureDelay<T>::loop() {
    int read_bytes;
    int available;
    int offset = 0;

    static long total_samples = 0;
    static long delay_next_save = 0;
    static int delay_next_slot = 0;

    while (run) {
        available = std::min(this->writer->writeable(), (size_t) 1024) * sizeof(T) - offset;
        read_bytes = recv(sock, ((char*) this->writer->getWritePointer()) + offset, available, 0);
        if (read_bytes <= 0) {
            run = false;
        } else {
            size_t samples = (offset + read_bytes) / sizeof(T);
            this->writer->advance(samples);
            offset = (offset + read_bytes) % sizeof(T);

            total_samples += samples;
            if (total_samples > delay_next_save) {
                clock_gettime(CLOCK_REALTIME, &delay_slots[delay_next_slot]);
                // WARNING - sample rate is hardcoded
                delay_next_save += long(2000000 * delay_interval);
                delay_next_slot = (delay_next_slot + 1) % num_delay_slots;
            }
        }
    }
}

template <typename T>
void TcpSourceMeasureDelay<T>::stop() {
    run = false;
    if (thread != nullptr) {
        thread->join();
        delete(thread);
    }
}

namespace Csdrx {
    template class TcpSourceMeasureDelay<unsigned char>;
    template class TcpSourceMeasureDelay<short>;
    template class TcpSourceMeasureDelay<float>;
    template class TcpSourceMeasureDelay<Csdr::complex<short>>;
    template class TcpSourceMeasureDelay<Csdr::complex<float>>;
}
