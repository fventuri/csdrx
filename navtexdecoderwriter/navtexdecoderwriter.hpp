/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/writer.hpp>
#include <navtex_rx.h>

#include <stdio.h>

namespace Csdrx {

    template <typename T>
    class NavtexDecoderWriter: public Csdr::Writer<T> {
        public:
            NavtexDecoderWriter(unsigned int samplerate, FILE* outfile,
                                bool reverse = false, size_t buffer_size = 8192);
            ~NavtexDecoderWriter();
            size_t writeable() override;
            T* getWritePointer() override;
            void advance(size_t how_much) override;
        private:
            size_t buffer_size;
            T* buffer;
            navtex_rx* nv;
    };
}
