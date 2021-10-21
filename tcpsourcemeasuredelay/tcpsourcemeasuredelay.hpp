/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/source.hpp>
#include <csdr/writer.hpp>

#include <arpa/inet.h>
#include <stdexcept>
#include <thread>

namespace Csdrx {

    template <typename T>
    class TcpSourceMeasureDelay: public Csdr::Source<T> {
        public:
            // TcpSourceMeasureDelay(std::string remote);
            TcpSourceMeasureDelay(in_addr_t ip, unsigned short port);
            TcpSourceMeasureDelay(unsigned short port);
            ~TcpSourceMeasureDelay();
            void setWriter(Csdr::Writer<T>* writer) override;
            void stop();
        private:
            void loop();
            int sock;
            bool run = true;
            std::thread* thread = nullptr;
    };

}
