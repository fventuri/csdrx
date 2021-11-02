/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dsddecoder.hpp"

using namespace Csdrx;

DsdDecoder::DsdDecoder(const char *mode, FILE* formatTextFile, bool upsample):
    slots(1),
    formatTextFile(formatTextFile),
    formatTextRefresh(0.1f)
{

    setDecodeMode(mode);
    if (upsample) {
        setUpsampling(6);   // 8k x 6 = 48k
    }
    nbFormatTextSamplesLeft = 48000.0f * formatTextRefresh;
}

DsdDecoder::~DsdDecoder()
{}

void DsdDecoder::process() {
    std::lock_guard<std::mutex> lock(processMutex);
    size_t available = reader->available();

    int nbAudioSamples1 = 0, nbAudioSamples2 = 0;
    short *audioSamples1, *audioSamples2;

    int nbSamples = 0;
    auto samples = reader->getReadPointer();
    for (nbSamples = 0; nbSamples < available; nbSamples++) {
        dsdDecoder.run(samples[nbSamples]);
        if (slots & 1) {
            audioSamples1 = dsdDecoder.getAudio1(nbAudioSamples1);
        }
        if (slots & 2) {
            audioSamples2 = dsdDecoder.getAudio2(nbAudioSamples2);
        }
        if (nbAudioSamples1 > 0 && nbAudioSamples2 == 0) {
            size_t writeable = std::min(writer->writeable(), (size_t)nbAudioSamples1);
            memcpy(writer->getWritePointer(), audioSamples1, writeable * sizeof(short));
            writer->advance(writeable);
            dsdDecoder.resetAudio1();
        } else if (nbAudioSamples1 == 0 && nbAudioSamples2 > 0) {
            size_t writeable = std::min(writer->writeable(), (size_t)nbAudioSamples2);
            memcpy(writer->getWritePointer(), audioSamples2, writeable * sizeof(short));
            writer->advance(writeable);
            dsdDecoder.resetAudio2();
        } else if (nbAudioSamples1 > 0 && nbAudioSamples2 > 0) {
            std::cerr << "both channels have audio - playing channel #1" << std::endl;
            size_t writeable = std::min(writer->writeable(), (size_t)nbAudioSamples1);
            memcpy(writer->getWritePointer(), audioSamples1, writeable * sizeof(short));
            writer->advance(writeable);
            dsdDecoder.resetAudio1();
            dsdDecoder.resetAudio2();
        }

        if (formatTextFile != nullptr) {
            nbFormatTextSamplesLeft--;
            if (nbFormatTextSamplesLeft == 0) {
                char formatText[128];
                dsdDecoder.formatStatusText(formatText);
                fputs(formatText, formatTextFile);
                putc('\n', formatTextFile);
                nbFormatTextSamplesLeft = 48000.0f * formatTextRefresh;
            }
        }
    }
    reader->advance(available);
}

bool DsdDecoder::canProcess() {
    std::lock_guard<std::mutex> lock(processMutex);
    size_t available = reader->available();
    size_t writeable = writer->writeable();
    return available > 0 && writeable > 0;
}

// setters
void DsdDecoder::setDecodeMode(DSDcc::DSDDecoder::DSDDecodeMode mode, bool on) {
    dsdDecoder.setDecodeMode(DSDcc::DSDDecoder::DSDDecodeAuto, false);
    dsdDecoder.setDecodeMode(mode, on);
}

void DsdDecoder::setDecodeMode(const char *mode) {
    if (strcmp(mode, "auto") == 0 || strcmp(mode, "AUTO") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeAuto, true);
    } else if (strcmp(mode, "dmr") == 0 || strcmp(mode, "DMR") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeDMR, true);
    } else if (strcmp(mode, "dstar") == 0 || strcmp(mode, "DStar") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeDStar, true);
    } else if (strcmp(mode, "x2tdma") == 0 || strcmp(mode, "X2TDMA") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeX2TDMA, true);
    } else if (strcmp(mode, "provoice") == 0 || strcmp(mode, "ProVoice") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeProVoice, true);
    } else if (strcmp(mode, "p25p1") == 0 || strcmp(mode, "P25P1") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeP25P1, true);
    } else if (strcmp(mode, "nxdn48") == 0 || strcmp(mode, "NXDN48") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeNXDN48, true);
    } else if (strcmp(mode, "nxdn96") == 0 || strcmp(mode, "NXDN96") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeNXDN96, true);
    } else if (strcmp(mode, "dpmr") == 0 || strcmp(mode, "DPMR") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeDPMR, true);
    } else if (strcmp(mode, "ysf") == 0 || strcmp(mode, "YSF") == 0) {
        setDecodeMode(DSDcc::DSDDecoder::DSDDecodeYSF, true);
    } else
        throw DSDException("invalid DSD mode");
}
