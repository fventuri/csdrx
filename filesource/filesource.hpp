/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/source.hpp>
#include <stdexcept>

namespace Csdrx {

    class IOException: public std::runtime_error {
        public:
            IOException(const std::string& reason): std::runtime_error(reason) {}
    };

    template <typename T>
    class FileSource: public Csdr::Source<T> {
        public:
            FileSource(const char* filename = nullptr, double samplerate = 0);
            ~FileSource();
            void setWriter(Csdr::Writer<T>* writer) override;
            void stop();
            bool isRunning() const;
        private:
            void loop();
            int fd;
            double samplerate;
            bool run = true;
            std::thread* thread = nullptr;
    };
}
