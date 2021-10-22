/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/source.hpp>

namespace Csdrx {

    template <typename T>
    class FileSourceMeasureDelay: public Csdr::Source<T> {
        public:
            FileSourceMeasureDelay(unsigned int delay_samplerate, const char* filename = nullptr, double samplerate = 0);
            ~FileSourceMeasureDelay();
            void setWriter(Csdr::Writer<T>* writer) override;
            void stop();
            bool isRunning() const;
        private:
            void loop();
            int fd;
            double samplerate;
            bool run = true;
            std::thread* thread = nullptr;
            unsigned int delay_samplerate;
    };
}
