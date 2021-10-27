/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/source.hpp>
#include <SoapySDR/Device.hpp>
#include <stdexcept>
#include <thread>

namespace Csdrx {

    class SoapyException: public std::runtime_error {
        public:
            SoapyException(const std::string& reason): std::runtime_error(reason) {}
    };

    template <typename T>
    class SoapySource: public Csdr::Source<T> {
        public:
            SoapySource(const std::string &args = "",
                        const size_t channel = 0,
                        double samplerate = 2e6,
                        double frequency = 100e6,
                        const std::string &antenna = "");
            ~SoapySource();
            void setWriter(Csdr::Writer<T>* writer) override;
            void stop();
            bool isRunning() const;
            void setChannel(const size_t channel);
            void setSamplerate(const double samplerate);
            void setBandwidth(const double bw);
            void setFrequency(const double frequency);
            void setAntenna(const std::string &antenna);
            void setGain(const double value);
            void setGain(const std::string& name, const double value);
            void setAGC(bool enable);
            void setDCOffset(bool enable);
            void setIQBalance(bool enable);
            void writeSetting(const std::string &key, const std::string &value);
            void writeChannelSetting(const std::string &key, const std::string &value);
        private:
            std::string get_stream_format() const;
            void loop();
            void show_device_config() const;
            SoapySDR::Device* device = nullptr;
            SoapySDR::Stream* stream = nullptr;
            size_t channel = 0;
            double samplerate;
            bool run = false;
            std::thread* thread = nullptr;
            size_t total_samples = 0;
    };
}
