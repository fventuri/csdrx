/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "navtexdecoderwriter.hpp"

//#include <unistd.h>

using namespace Csdrx;

template <typename T>
NavtexDecoderWriter<T>::NavtexDecoderWriter(unsigned int samplerate,
                                            FILE* outfile, bool reverse,
                                            size_t buffer_size):
    buffer_size(buffer_size),
    buffer((T*) malloc(sizeof(T) * buffer_size))
{
    // ignore the auxiliary stream (for now): set err=nullptr
    nv = new navtex_rx(samplerate, false, reverse, outfile, nullptr);
}

template <typename T>
NavtexDecoderWriter<T>::~NavtexDecoderWriter() {
    delete nv;
    free(buffer);
}

template <typename T>
size_t NavtexDecoderWriter<T>::writeable() {
    return buffer_size;
}

template <typename T>
T* NavtexDecoderWriter<T>::getWritePointer() {
    return buffer;
}

template <typename T>
void NavtexDecoderWriter<T>::advance(size_t how_much) {
    nv->process_data(buffer, how_much);
}

namespace Csdrx {
    template class NavtexDecoderWriter<float>;
}
