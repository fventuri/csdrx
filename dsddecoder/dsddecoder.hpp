/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <csdr/module.hpp>

#include <dsdcc/dsd_decoder.h>
#include <dsdcc/dsd_upsample.h>

namespace Csdrx {

    class DSDException: public std::runtime_error {
        public:
            DSDException(const std::string& reason): std::runtime_error(reason) {}
    };

    class DsdDecoder: public Csdr::Module<short, short> {
        public:
            DsdDecoder(const char *mode = "auto", FILE* formatTextFile = nullptr, bool upsample = false);
            ~DsdDecoder() override;
            bool canProcess() override;
            void process() override;
            // setters
            void setSlots(int slots) { this->slots = slots; }
            void setFormatTextFile(FILE* formatTextFile) { this->formatTextFile = formatTextFile; }
            void setFormatTextRefresh(float refresh) {
                formatTextRefresh = refresh;
                nbFormatTextSamplesLeft = 48000.0f * refresh;
            }
            void setLogVerbosity(int verbosity) { dsdDecoder.setLogVerbosity(verbosity); }
            void setLogFile(const char *filename) { dsdDecoder.setLogFile(filename); }
            void setTDMAStereo(bool tdmaStereo) { dsdDecoder.setTDMAStereo(tdmaStereo); }
            void setQuiet();
            void setVerbosity(int verbosity) { dsdDecoder.setVerbosity(verbosity); }
            void showErrorBars() { dsdDecoder.showErrorBars(); }
            void showSymbolTiming() { dsdDecoder.showSymbolTiming(); }
            void setP25DisplayOptions(DSDcc::DSDDecoder::DSDShowP25 mode, bool on) { dsdDecoder.setP25DisplayOptions(mode, on); }
            void muteEncryptedP25(bool on) { dsdDecoder.muteEncryptedP25(on); }
            void setDecodeMode(DSDcc::DSDDecoder::DSDDecodeMode mode, bool on);
            void setDecodeMode(const char *mode);
            void setAudioGain(float gain) { dsdDecoder.setAudioGain(gain); }
            void setUvQuality(int uvquality) { dsdDecoder.setUvQuality(uvquality); }
            void setUpsampling(int upsampling) { dsdDecoder.setUpsampling(upsampling); }
            void setStereo(bool on) { dsdDecoder.setStereo(on); }
            void setInvertedXTDMA(bool on) { dsdDecoder.setInvertedXTDMA(on); }
            void enableCosineFiltering(bool on) { dsdDecoder.enableCosineFiltering(on); }
            void enableAudioOut(bool on) { dsdDecoder.enableAudioOut(on); }
            void enableScanResumeAfterTDULCFrames(int nbFrames) { dsdDecoder.enableScanResumeAfterTDULCFrames(nbFrames); }
            void setDataRate(DSDcc::DSDDecoder::DSDRate dataRate) { dsdDecoder.setDataRate(dataRate); }
            void setMyPoint(float lat, float lon) { dsdDecoder.setMyPoint(lat, lon); }
            void setSymbolPLLLock(bool pllLock) { dsdDecoder.setSymbolPLLLock(pllLock); }
            void setDMRBasicPrivacyKey(unsigned char key) { dsdDecoder.setDMRBasicPrivacyKey(key); }
            void setMbeRate(DSDcc::DSDDecoder::DSDMBERate mbeRate) { dsdDecoder.setMbeRate(mbeRate); }
            void useHPMbelib(bool useHP) { dsdDecoder.useHPMbelib(useHP); }
        private:
            DSDcc::DSDDecoder dsdDecoder;
            DSDcc::DSDUpsampler upsamplingEngine;
            int slots;
            FILE* formatTextFile;
            float formatTextRefresh;
            int nbFormatTextSamplesLeft;
    };

}
