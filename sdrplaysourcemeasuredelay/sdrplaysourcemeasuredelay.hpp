/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/source.hpp>
#include <sdrplay_api.h>

namespace Csdrx {

    template <typename T>
    class SDRplaySourceMeasureDelay: public Csdr::Source<T> {
        public:
            SDRplaySourceMeasureDelay(const char* serial = nullptr,
                                      double samplerate = 2e6,
                                      double frequency = 100e6,
                                      const char* antenna = nullptr);
            ~SDRplaySourceMeasureDelay();
            void setWriter(Csdr::Writer<T>* writer) override;
            void stop();
            bool isRunning() const;
            // setters
            void setSamplerate(double samplerate);
            void setBandwidth(double samplerate);
            void setFrequency(double frequency);
            void setAntenna(const char* antenna);
            void setIFGainReduction(int gRdB);   // gRdB == 0 -> enable AGC
            void setRFLnaState(unsigned char LNAstate);
            void setIFType(int if_type);
            void setPPM(double ppm);
            void setDCOffset(bool enable);
            void setIQBalance(bool enable);
            // getters
            double getSamplerate() const;
            double getBandwidth() const;
            double getFrequency() const;
            const char* getAntenna() const;
            int getIFGainReduction() const;
            unsigned char getRFLnaState() const;
            int getIFType() const;
            double getPPM() const;
            bool getDCOffset() const;
            bool getIQBalance() const;
            void stream_callback(short *xi, short *xq,
                                 sdrplay_api_StreamCbParamsT *params,
                                 unsigned int numSamples, unsigned int reset);
        private:
            void select_device(const char* serial, const char* antenna);
            bool select_device_rspduo(sdrplay_api_DeviceT& device_rspduo,
                                      const char* serial, const char* antenna);
            sdrplay_api_StreamCallback_t get_unqualified_stream_callback() const;
            void show_device_config() const;
            sdrplay_api_DeviceT device;
            sdrplay_api_DeviceParamsT *device_params;
            sdrplay_api_RxChannelParamsT *rx_channel_params;
            double samplerate;
            bool run = false;
            bool device_selected = false;
            size_t total_samples = 0;
    };
}
