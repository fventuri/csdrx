/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "soapysource.hpp"

#include <SoapySDR/Formats.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace Csdrx;

template<typename T>
SoapySource<T>::~SoapySource() {
    SoapySDR::Device::unmake(device);
    device = nullptr;
}

template <typename T>
SoapySource<T>::SoapySource(const std::string &args,
                            const size_t channel,
                            double samplerate,
                            double frequency,
                            const std::string &antenna):
    channel(channel)
{
    device = SoapySDR::Device::make(args);
    std::string format = get_stream_format();
    std::vector<std::string> stream_formats = device->getStreamFormats(SOAPY_SDR_RX, channel);
    if (std::find(stream_formats.begin(), stream_formats.end(), format) == stream_formats.end())
        throw SoapyException("invalid stream format");
    setSamplerate(samplerate);
    setBandwidth(samplerate);
    setFrequency(frequency);
    setAntenna(antenna);
}

template<>
std::string SoapySource<Csdr::complex<float>>::get_stream_format() const {
    return SOAPY_SDR_CF32;
}

template<>
std::string SoapySource<Csdr::complex<short>>::get_stream_format() const {
    return SOAPY_SDR_CS16;
}

template <typename T>
void SoapySource<T>::setWriter(Csdr::Writer<T> *writer) {
    Csdr::Source<T>::setWriter(writer);
    stream = device->setupStream(SOAPY_SDR_RX, get_stream_format(),
                                 std::vector<size_t>{channel});
    //show_device_config();
    device->activateStream(stream);
    run = true;
    if (thread == nullptr) {
        thread = new std::thread( [this] () { loop(); });
    }
}

template <typename T>
void SoapySource<T>::loop() {

    size_t available;
    long long timeNs = 0;
    long timeoutNs = 1e6;
    int flags = 0;

    total_samples = 0;
    while (run) {
        available = std::min(this->writer->writeable(), (size_t) 1024);
        void* buffs[] = {(void*) this->writer->getWritePointer()};
        int samples = device->readStream(stream, buffs, available, flags, timeNs, timeoutNs);
        if (samples > 0) {
            this->writer->advance(samples);
            total_samples += samples;
        } else if (samples == SOAPY_SDR_OVERFLOW) {
            // overflows do happen, they are non-fatal. a warning should do
            std::cerr << "WARNING: SoapySDR::Device::readStream overflow" << std::endl;
        } else if (samples == SOAPY_SDR_TIMEOUT) {
            // timeout should not break the read loop.
            // TODO or should they? Jakob Ketterl tried, but airspyhf devices will end up here on sample rate changes.
            std::cerr << "WARNING: SoapySDR::Device::readStream timeout!" << std::endl;
        } else {
            // other errors should break the read loop.
            std::cerr << "ERROR: SoapySDR::Device::readStream error " << samples << std::endl;
            break;
        }
    }
}

template <typename T>
void SoapySource<T>::stop() {
    if (!run)
        return;

    run = false;
    if (thread != nullptr) {
        thread->join();
        delete(thread);
        thread = nullptr;
    }
    device->deactivateStream(stream);
    device->closeStream(stream);
    stream = nullptr;
    std::cerr << "total_samples: " << total_samples << std::endl;
}

template <typename T>
bool SoapySource<T>::isRunning() const {
    return run;
}

template <typename T>
void SoapySource<T>::setChannel(const size_t channel)
{
    this->channel = channel;
}

template <typename T>
void SoapySource<T>::setSamplerate(const double samplerate)
{
    device->setSampleRate(SOAPY_SDR_RX, channel, samplerate);
}

template <typename T>
void SoapySource<T>::setBandwidth(const double bw)
{
    double bwreq = 0;
    for (auto& bandwidth : device->listBandwidths(SOAPY_SDR_RX, channel)) {
        if (bandwidth > bw) {
            if (bwreq == 0)
                bwreq = bandwidth;
            break;
        }
        bwreq = bandwidth;
    }
    device->setBandwidth(SOAPY_SDR_RX, channel, bwreq);
}

template <typename T>
void SoapySource<T>::setFrequency(const double frequency)
{
    device->setFrequency(SOAPY_SDR_RX, channel, frequency);
}

template <typename T>
void SoapySource<T>::setAntenna(const std::string &antenna)
{
    device->setAntenna(SOAPY_SDR_RX, channel, antenna);
}

template <typename T>
void SoapySource<T>::setGain(const double value)
{
    device->setGain(SOAPY_SDR_RX, channel, value);
}

template <typename T>
void SoapySource<T>::setGain(const std::string& name, const double value)
{
    device->setGain(SOAPY_SDR_RX, channel, name, value);
}

template <typename T>
void SoapySource<T>::setAGC(bool enable)
{
    device->setGainMode(SOAPY_SDR_RX, channel, enable);
}

template <typename T>
void SoapySource<T>::setDCOffset(bool enable)
{
    device->setDCOffsetMode(SOAPY_SDR_RX, channel, enable);
}

#if defined(SOAPY_SDR_API_VERSION) && (SOAPY_SDR_API_VERSION >= 0x00080000)
template <typename T>
void SoapySource<T>::setIQBalance(bool enable)
{
    device->setIQBalanceMode(SOAPY_SDR_RX, channel, enable);
}
#endif

template <typename T>
void SoapySource<T>::writeSetting(const std::string &key, const std::string &value)
{
    device->writeSetting(key, value);
}

template <typename T>
void SoapySource<T>::writeChannelSetting(const std::string &key, const std::string &value)
{
    device->writeSetting(SOAPY_SDR_RX, channel, key, value);
}

template <typename T>
void SoapySource<T>::show_device_config() const
{
    std::cerr << "--------------------------------------------------------------------------------" << std::endl;
    std::cerr << "    sample rate=" << device->getSampleRate(SOAPY_SDR_RX, channel) << std::endl;
    std::cerr << "    frequency=" << device->getFrequency(SOAPY_SDR_RX, channel) << std::endl;
    std::cerr << "    bandwidth=" << device->getBandwidth(SOAPY_SDR_RX, channel) << std::endl;
    std::cerr << "    gain mode=" << device->getGainMode(SOAPY_SDR_RX, channel) << std::endl;
    std::cerr << "    gain(RFGR)=" << device->getGain(SOAPY_SDR_RX, channel, "RFGR") << std::endl;
    std::cerr << "    gain(IFGR)=" << device->getGain(SOAPY_SDR_RX, channel, "IFGR") << std::endl;
    std::cerr << "    antenna=" << device->getAntenna(SOAPY_SDR_RX, channel) << std::endl;
    std::cerr << "--------------------------------------------------------------------------------" << std::endl;
}

namespace Csdrx {
    template class SoapySource<Csdr::complex<short>>;
    template class SoapySource<Csdr::complex<float>>;
}
