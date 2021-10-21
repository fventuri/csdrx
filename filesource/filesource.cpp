/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "filesource.hpp"

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
FileSource<T>::~FileSource() {
    ::close(fd);
}

template <typename T>
FileSource<T>::FileSource(const char* filename, double samplerate) {

    if (filename == nullptr || strcmp(filename, "") == 0 || strcmp(filename, "-") == 0) {
        // default reads from stdin
        fd = fileno(stdin);
    } else {
        fd = open(filename, O_RDONLY);
        if (fd < 0)
            throw IOException("unable to open file for reading");
    }

    this->samplerate = samplerate;
}

template <typename T>
void FileSource<T>::setWriter(Csdr::Writer<T> *writer) {
    Csdr::Source<T>::setWriter(writer);
    if (thread == nullptr) {
        thread = new std::thread( [this] () { loop(); });
    }
}

template <typename T>
void FileSource<T>::loop() {
    int read_bytes;
    int available;
    int offset = 0;
    size_t total_samples = 0;
    struct timespec start_time;

    static long delay_next_save = 0;
    static int delay_next_slot = 0;

    while (run) {
        available = std::min(this->writer->writeable(), (size_t) 1024) * sizeof(T) - offset;
        if (samplerate > 0) {
            if (total_samples == 0) {
                clock_gettime(CLOCK_REALTIME, &start_time);
            } else {
                double total_delay_s;
                double total_delay_frac = modf(total_samples / samplerate,
                                               &total_delay_s);
                long tv_nsec = start_time.tv_nsec + long(total_delay_frac * 1e9);
                long tv_sec = start_time.tv_sec + long(total_delay_s) + tv_nsec / 1000000000;
                tv_nsec %= 1000000000;
                struct timespec request_time = { tv_sec, tv_nsec };
                clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                                &request_time, nullptr);
            }
        }
        read_bytes = read(fd, ((char*) this->writer->getWritePointer()) + offset, available);
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
void FileSource<T>::stop() {
    run = false;
    if (thread != nullptr) {
        thread->join();
        delete(thread);
    }
}

template <typename T>
bool FileSource<T>::isRunning() const {
    return run;
}

namespace Csdrx {
    template class FileSource<unsigned char>;
    template class FileSource<short>;
    template class FileSource<float>;
    template class FileSource<Csdr::complex<short>>;
    template class FileSource<Csdr::complex<float>>;
}
