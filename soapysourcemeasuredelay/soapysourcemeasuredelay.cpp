/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "soapysourcemeasuredelay.hpp"
// needed for the SoapyException class
#include "../soapysource/soapysource.hpp"

#include <SoapySDR/Formats.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

extern const int num_delay_slots;
extern struct timespec delay_slots[];
extern const double delay_interval;

using namespace Csdrx;

template<typename T>
SoapySourceMeasureDelay<T>::~SoapySourceMeasureDelay() {
    SoapySDR::Device::unmake(device);
    device = nullptr;
}

template <typename T>
SoapySourceMeasureDelay<T>::SoapySourceMeasureDelay(const std::string &args,
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
std::string SoapySourceMeasureDelay<Csdr::complex<float>>::get_stream_format() const {
    return SOAPY_SDR_CF32;
}

template<>
std::string SoapySourceMeasureDelay<Csdr::complex<short>>::get_stream_format() const {
    return SOAPY_SDR_CS16;
}

template <typename T>
void SoapySourceMeasureDelay<T>::setWriter(Csdr::Writer<T> *writer) {
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
void SoapySourceMeasureDelay<T>::loop() {

    size_t available;
    long long timeNs = 0;
    long timeoutNs = 1e6;
    int flags = 0;

    static long delay_next_save = 0;
    static int delay_next_slot = 0;

    total_samples = 0;
    while (run) {
        available = std::min(this->writer->writeable(), (size_t) 1024);
        void* buffs[] = {(void*) this->writer->getWritePointer()};
        int samples = device->readStream(stream, buffs, available, flags, timeNs, timeoutNs);
        if (samples > 0) {
            this->writer->advance(samples);
            total_samples += samples;
            if (total_samples > delay_next_save) {
                clock_gettime(CLOCK_REALTIME, &delay_slots[delay_next_slot]);
                delay_next_save += long(samplerate * delay_interval);
                delay_next_slot = (delay_next_slot + 1) % num_delay_slots;
            }
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
void SoapySourceMeasureDelay<T>::stop() {
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
bool SoapySourceMeasureDelay<T>::isRunning() const {
    return run;
}

// setters
template <typename T>
void SoapySourceMeasureDelay<T>::setChannel(const size_t channel)
{
    this->channel = channel;
}

template <typename T>
void SoapySourceMeasureDelay<T>::setSamplerate(const double samplerate)
{
    device->setSampleRate(SOAPY_SDR_RX, channel, samplerate);
}

template <typename T>
void SoapySourceMeasureDelay<T>::setBandwidth(const double bw)
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
void SoapySourceMeasureDelay<T>::setFrequency(const double frequency)
{
    device->setFrequency(SOAPY_SDR_RX, channel, frequency);
}

template <typename T>
void SoapySourceMeasureDelay<T>::setAntenna(const std::string &antenna)
{
    device->setAntenna(SOAPY_SDR_RX, channel, antenna);
}

template <typename T>
void SoapySourceMeasureDelay<T>::setGain(const double value)
{
    device->setGain(SOAPY_SDR_RX, channel, value);
}

template <typename T>
void SoapySourceMeasureDelay<T>::setGain(const std::string& name, const double value)
{
    device->setGain(SOAPY_SDR_RX, channel, name, value);
}

template <typename T>
void SoapySourceMeasureDelay<T>::setAGC(bool enable)
{
    device->setGainMode(SOAPY_SDR_RX, channel, enable);
}

template <typename T>
void SoapySourceMeasureDelay<T>::setPPM(const double ppm)
{
#if defined(SOAPY_SDR_API_VERSION) && (SOAPY_SDR_API_VERSION >= 0x00060000)
    device->setFrequencyCorrection(SOAPY_SDR_RX, channel, ppm);
#else
    device->setFrequency(SOAPY_SDR_RX, channel, "CORR", ppm);
#endif
}

template <typename T>
void SoapySourceMeasureDelay<T>::setDCOffset(bool enable)
{
    device->setDCOffsetMode(SOAPY_SDR_RX, channel, enable);
}

#if defined(SOAPY_SDR_API_VERSION) && (SOAPY_SDR_API_VERSION >= 0x00080000)
template <typename T>
void SoapySourceMeasureDelay<T>::setIQBalance(bool enable)
{
    device->setIQBalanceMode(SOAPY_SDR_RX, channel, enable);
}
#endif

template <typename T>
void SoapySourceMeasureDelay<T>::writeSetting(const std::string &key, const std::string &value)
{
    device->writeSetting(key, value);
}

template <typename T>
void SoapySourceMeasureDelay<T>::writeChannelSetting(const std::string &key, const std::string &value)
{
    device->writeSetting(SOAPY_SDR_RX, channel, key, value);
}

// getters
template <typename T>
size_t SoapySourceMeasureDelay<T>::getChannel() const
{
    return channel;
}

template <typename T>
double SoapySourceMeasureDelay<T>::getSamplerate() const
{
    return device->getSampleRate(SOAPY_SDR_RX, channel);
}

template <typename T>
double SoapySourceMeasureDelay<T>::getBandwidth() const
{
    return device->getBandwidth(SOAPY_SDR_RX, channel);
}

template <typename T>
double SoapySourceMeasureDelay<T>::getFrequency() const
{
    return device->getFrequency(SOAPY_SDR_RX, channel);
}

template <typename T>
std::string SoapySourceMeasureDelay<T>::getAntenna() const
{
    return device->getAntenna(SOAPY_SDR_RX, channel);
}

template <typename T>
double SoapySourceMeasureDelay<T>::getGain() const
{
    return device->getGain(SOAPY_SDR_RX, channel);
}

template <typename T>
double SoapySourceMeasureDelay<T>::getGain(const std::string& name) const
{
    return device->getGain(SOAPY_SDR_RX, channel, name);
}

template <typename T>
bool SoapySourceMeasureDelay<T>::getAGC() const
{
    return device->getGainMode(SOAPY_SDR_RX, channel);
}

template <typename T>
double SoapySourceMeasureDelay<T>::getPPM() const
{
#if defined(SOAPY_SDR_API_VERSION) && (SOAPY_SDR_API_VERSION >= 0x00060000)
    return device->getFrequencyCorrection(SOAPY_SDR_RX, channel);
#else
    return device->getFrequency(SOAPY_SDR_RX, channel, "CORR");
#endif
}

template <typename T>
bool SoapySourceMeasureDelay<T>::getDCOffset() const
{
    return device->getDCOffsetMode(SOAPY_SDR_RX, channel);
}

#if defined(SOAPY_SDR_API_VERSION) && (SOAPY_SDR_API_VERSION >= 0x00080000)
template <typename T>
bool SoapySourceMeasureDelay<T>::getIQBalance() const
{
    return device->getIQBalanceMode(SOAPY_SDR_RX, channel);
}
#endif

template <typename T>
std::string SoapySourceMeasureDelay<T>::readSetting(const std::string &key) const
{
    return device->readSetting(key);
}

template <typename T>
std::string SoapySourceMeasureDelay<T>::readChannelSetting(const std::string &key) const
{
    return device->readSetting(SOAPY_SDR_RX, channel, key);
}

template <typename T>
void SoapySourceMeasureDelay<T>::show_device_config() const
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
    template class SoapySourceMeasureDelay<Csdr::complex<short>>;
    template class SoapySourceMeasureDelay<Csdr::complex<float>>;
}
