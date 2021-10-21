/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pulseaudiowriter.hpp"

#include <unistd.h>
#include <pulse/error.h>

#include <iostream>
extern const int num_delay_slots;
extern struct timespec delay_slots[];
extern const double delay_interval;

using namespace Csdrx;

template <typename T>
PulseAudioWriter<T>::PulseAudioWriter(unsigned int samplerate, size_t buffer_size,
                             const char* app_name, const char* stream_name):
    buffer_size(buffer_size),
    buffer((T*) malloc(sizeof(T) * buffer_size))
{
    static const pa_sample_spec sample_spec = {
        .format = PA_SAMPLE_S16LE,
        .rate = samplerate,
        .channels = 1
    };

    int error;
    if (!(pa = pa_simple_new(nullptr, app_name == nullptr ? "csdr" : app_name,
                             PA_STREAM_PLAYBACK, nullptr,
                             stream_name == nullptr ? "pawriter0" : stream_name,
                             &sample_spec, nullptr, nullptr, &error)))
        throw std::runtime_error(std::string("pa_simple_new() failed: ") + pa_strerror(error));
}

template <typename T>
PulseAudioWriter<T>::~PulseAudioWriter() {
    int error;
    pa_simple_drain(pa, &error);
    pa_simple_free(pa);
    free(buffer);
}

template <typename T>
size_t PulseAudioWriter<T>::writeable() {
    return buffer_size;
}

template <typename T>
T* PulseAudioWriter<T>::getWritePointer() {
    return buffer;
}

template <typename T>
void PulseAudioWriter<T>::advance(size_t how_much) {
    static long total_samples = 0;
    static long delay_next_print = 0;
    static int delay_next_slot = 0;
    total_samples += how_much;
    if (total_samples > delay_next_print) {
        struct timespec end_time;
        clock_gettime(CLOCK_REALTIME, &end_time);
        long delay_ns = (end_time.tv_sec - delay_slots[delay_next_slot].tv_sec) * 1000000 + (end_time.tv_nsec - delay_slots[delay_next_slot].tv_nsec);
        time_t now = time(nullptr);
        std::cerr.write(ctime(&now), 24);
        std::cerr << " - delay=" << (delay_ns / 1e6) << " ms" << std::endl;
        // WARNING - sample rate is hardcoded
        delay_next_print += long(48000 * delay_interval);
        delay_next_slot = (delay_next_slot + 1) % num_delay_slots;
    }

    int error;
    pa_simple_write(pa, (const uint8_t*) buffer, sizeof(T) * how_much, &error);
}

namespace Csdrx {
    template class PulseAudioWriter<short>;
}
