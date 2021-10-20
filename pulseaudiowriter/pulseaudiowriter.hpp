/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/writer.hpp>
#include <pulse/simple.h>

namespace Csdrx {

    template <typename T>
    class PulseAudioWriter: public Csdr::Writer<T> {
        public:
            PulseAudioWriter(unsigned int samplerate, size_t buffer_size = 10240,
                             const char* app_name = nullptr,
                             const char* stream_name = nullptr);
            ~PulseAudioWriter();
            size_t writeable() override;
            T* getWritePointer() override;
            void advance(size_t how_much) override;
        private:
            size_t buffer_size;
            T* buffer;
            pa_simple *pa;
    };
}
