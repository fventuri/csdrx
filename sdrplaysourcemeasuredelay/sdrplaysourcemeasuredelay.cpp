/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sdrplaysourcemeasuredelay.hpp"
// needed for the SDRplayException class
#include "../sdrplaysource/sdrplaysource.hpp"

#include <cstring>
#include <iostream>

extern const int num_delay_slots;
extern struct timespec delay_slots[];
extern const double delay_interval;

using namespace Csdrx;

template<typename T>
SDRplaySourceMeasureDelay<T>::~SDRplaySourceMeasureDelay() {
    if (device_selected) {
        sdrplay_api_LockDeviceApi();
        auto err = sdrplay_api_ReleaseDevice(&device);
        if (err != sdrplay_api_Success)
            std::cerr << "sdrplay_api_ReleaseDevice() failed" << std::endl;
        sdrplay_api_UnlockDeviceApi();
    }
    auto err = sdrplay_api_Close();
    if (err != sdrplay_api_Success)
        std::cerr << "sdrplay_api_Close() failed" << std::endl;
}

static void open_sdrplay_api() {
    auto err = sdrplay_api_Open();
    if (err != sdrplay_api_Success)
        throw SDRplayException("sdrplay_api_Open() failed");
    float ver;
    err = sdrplay_api_ApiVersion(&ver);
    if (err != sdrplay_api_Success) {
        sdrplay_api_Close();
        throw SDRplayException("sdrplay_api_ApiVersion() failed");
    }
    if (ver != SDRPLAY_API_VERSION) {
        sdrplay_api_Close();
        throw SDRplayException("SDRplay API version mismatch");
    }
}

template <typename T>
SDRplaySourceMeasureDelay<T>::SDRplaySourceMeasureDelay(const char* serial,
                                double samplerate,
                                double frequency,
                                const char* antenna)
{
    open_sdrplay_api();
    select_device(serial, antenna);
    setSamplerate(samplerate);
    setBandwidth(samplerate);
    setFrequency(frequency);
    setAntenna(antenna);
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::select_device(const char* serial,
                                     const char* antenna)
{
    auto err = sdrplay_api_LockDeviceApi();
    if (err != sdrplay_api_Success)
        throw SDRplayException("sdrplay_api_LockDeviceApi() failed");

    unsigned int ndevices = SDRPLAY_MAX_DEVICES;
    sdrplay_api_DeviceT devices[SDRPLAY_MAX_DEVICES];
    err = sdrplay_api_GetDevices(devices, &ndevices, ndevices);
    if (err != sdrplay_api_Success)
        throw SDRplayException("sdrplay_api_GetDevices() failed");

    bool found = false;
    for (int i = 0; i < ndevices; i++) {
        if (devices[i].hwVer == SDRPLAY_RSPduo_ID) {
            if (select_device_rspduo(devices[i], serial, antenna)) {
                found = true;
                break;
            }
        } else {
            if (serial == nullptr || strlen(serial) == 0 ||
                strcmp(devices[i].SerNo, serial) == 0) {
                found = true;
                device = devices[i];
                break;
            }
        }
    }

    if (!found)
        throw SDRplayException("SDRplay device not found");

    // select the device and get its parameters
    err = sdrplay_api_SelectDevice(&device);
    if (err != sdrplay_api_Success)
        throw SDRplayException("sdrplay_api_SelectDevice() failed");
    device_selected = true;
    err = sdrplay_api_UnlockDeviceApi();
    if (err != sdrplay_api_Success)
        throw SDRplayException("sdrplay_api_UnlockDeviceApi() failed");
    err = sdrplay_api_GetDeviceParams(device.dev, &device_params);
    if (err != sdrplay_api_Success)
        throw SDRplayException("sdrplay_api_GetDeviceParams() failed");
    rx_channel_params = device.tuner != sdrplay_api_Tuner_B ?
                                        device_params->rxChannelA :
                                        device_params->rxChannelB;
    samplerate = device_params->devParams->fsFreq.fsHz;
    if (device.hwVer == SDRPLAY_RSPduo_ID &&
        device.rspDuoMode != sdrplay_api_RspDuoMode_Single_Tuner)
        samplerate = 2e6;
    if (rx_channel_params->ctrlParams.decimation.enable)
        samplerate /= rx_channel_params->ctrlParams.decimation.decimationFactor;

    return;
}

template <typename T>
bool SDRplaySourceMeasureDelay<T>::select_device_rspduo(sdrplay_api_DeviceT& device_rspduo,
                                            const char* serial,
                                            const char* antenna) {
    bool found = false;
    if (device_rspduo.rspDuoMode & sdrplay_api_RspDuoMode_Single_Tuner) {
        if (serial == nullptr || strlen(serial) == 0 ||
            strcmp(device_rspduo.SerNo, serial) == 0) {
            found = true;
            device = device_rspduo;
            device.rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;
        }
    } else if (device_rspduo.rspDuoMode & sdrplay_api_RspDuoMode_Master) {
       size_t len = strlen(device_rspduo.SerNo);
        if (strncmp(device_rspduo.SerNo, serial, len) == 0) {
            if (strcmp("/M", serial + len) == 0) {
                found = true;
                device = device_rspduo;
                device.rspDuoMode = sdrplay_api_RspDuoMode_Master;
                device.rspDuoSampleFreq = 6e6;
            } else if (strcmp("/M8", serial + len) == 0) {
                found = true;
                device = device_rspduo;
                device.rspDuoMode = sdrplay_api_RspDuoMode_Master;
                device.rspDuoSampleFreq = 8e6;
            }
        }
    } else if (device_rspduo.rspDuoMode == sdrplay_api_RspDuoMode_Slave) {
        found = true;
        device = device_rspduo;
    }

    if (!found)
        return found;

    if (device.rspDuoMode == sdrplay_api_RspDuoMode_Single_Tuner ||
        device.rspDuoMode == sdrplay_api_RspDuoMode_Master) {
       device.tuner = sdrplay_api_Tuner_A;
       if (antenna != nullptr && strncmp(antenna, "Tuner 2", 7) == 0)
           device.tuner = sdrplay_api_Tuner_B;
    }

    return found;
}
static constexpr int MAX_WRITE_TRIES = 3;

template<>
void SDRplaySourceMeasureDelay<Csdr::complex<float>>::stream_callback(short *xi, short *xq,
                  sdrplay_api_StreamCbParamsT *params, unsigned int numSamples,
                  unsigned int reset) {

    static long delay_next_save = 0;
    static int delay_next_slot = 0;

    if (!run)
        return;

    int xidx = 0;
    for (int i = 0; i < MAX_WRITE_TRIES; i++) {
        int samples = std::min((int) writer->writeable(), (int) numSamples - xidx);
        auto writer_pointer = writer->getWritePointer();
        for (int k = 0; k < samples; k++, xidx++) {
            writer_pointer[k].real(static_cast<float>(xi[xidx]) / 32768.0f);
            writer_pointer[k].imag(static_cast<float>(xq[xidx]) / 32768.0f);
        }
        writer->advance(samples);
        total_samples += samples;
        if (total_samples > delay_next_save) {
             clock_gettime(CLOCK_REALTIME, &delay_slots[delay_next_slot]);
             delay_next_save += long(samplerate * delay_interval);
             delay_next_slot = (delay_next_slot + 1) % num_delay_slots;
        }
        if (xidx == numSamples)
            return;
    }

    std::cerr << "stream_callback() - dropped " << (numSamples - xidx) << " samples" << std::endl;

    return;
}

static void unqualified_stream_callback_fc32(short *xi, short *xq,
                                             sdrplay_api_StreamCbParamsT *params,
                                             unsigned int numSamples,
                                             unsigned int reset, void *cbContext) {
    auto sdrplay_source = static_cast<SDRplaySourceMeasureDelay<Csdr::complex<float>> *>(cbContext);
    sdrplay_source->stream_callback(xi, xq, params, numSamples, reset);
    return;
}

template<>
void SDRplaySourceMeasureDelay<Csdr::complex<short>>::stream_callback(short *xi, short *xq,
                  sdrplay_api_StreamCbParamsT *params, unsigned int numSamples,
                  unsigned int reset) {

    static long delay_next_save = 0;
    static int delay_next_slot = 0;

    if (!run)
        return;

    int xidx = 0;
    for (int i = 0; i < MAX_WRITE_TRIES; i++) {
        int samples = std::min((int) writer->writeable(), (int) numSamples - xidx);
        auto writer_pointer = writer->getWritePointer();
        for (int k = 0; k < samples; k++, xidx++) {
            writer_pointer[k].real(xi[xidx]);
            writer_pointer[k].imag(xq[xidx]);
        }
        writer->advance(samples);
        if (total_samples > delay_next_save) {
             clock_gettime(CLOCK_REALTIME, &delay_slots[delay_next_slot]);
             delay_next_save += long(samplerate * delay_interval);
             delay_next_slot = (delay_next_slot + 1) % num_delay_slots;
        }
        if (xidx == numSamples)
            return;
    }

    std::cerr << "stream_callback() - dropped " << (numSamples - xidx) << " samples" << std::endl;

    return;
}

static void unqualified_stream_callback_sc16(short *xi, short *xq,
                                             sdrplay_api_StreamCbParamsT *params,
                                             unsigned int numSamples,
                                             unsigned int reset, void *cbContext) {
    auto sdrplay_source = static_cast<SDRplaySourceMeasureDelay<Csdr::complex<short>> *>(cbContext);
    sdrplay_source->stream_callback(xi, xq, params, numSamples, reset);
    return;
}

static void event_callback(sdrplay_api_EventT eventId,
                           sdrplay_api_TunerSelectT tuner,
                           sdrplay_api_EventParamsT *params, void *cbContext)
{
    return;
}

template<>
sdrplay_api_StreamCallback_t SDRplaySourceMeasureDelay<Csdr::complex<float>>::get_unqualified_stream_callback() const {
    return unqualified_stream_callback_fc32;
}

template<>
sdrplay_api_StreamCallback_t SDRplaySourceMeasureDelay<Csdr::complex<short>>::get_unqualified_stream_callback() const {
    return unqualified_stream_callback_sc16;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setWriter(Csdr::Writer<T> *writer) {
    Csdr::Source<T>::setWriter(writer);
    sdrplay_api_CallbackFnsT callbackFns = {
        get_unqualified_stream_callback(),
        nullptr,
        event_callback,
    };
    //show_device_config();
    auto err = sdrplay_api_Init(device.dev, &callbackFns, this);
    if (err != sdrplay_api_Success)
        throw SDRplayException("sdrplay_api_Init() failed");
    total_samples = 0;
    run = true;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::stop() {
    if (run) {
        auto err = sdrplay_api_Uninit(device.dev);
        if (err != sdrplay_api_Success)
            throw SDRplayException("sdrplay_api_Uninit() failed");
    }
    run = false;
    std::cerr << "total_samples: " << total_samples << std::endl;
}

template <typename T>
bool SDRplaySourceMeasureDelay<T>::isRunning() const {
    return run;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setSamplerate(double samplerate)
{
    double fsHz = samplerate;
    unsigned char decimationFactor = 1;
    while (fsHz < 2e6 && decimationFactor <= 32) {
        fsHz *= 2;
        decimationFactor *= 2;
    }
    if (fsHz < 2e6 || fsHz > 10.66e6 || (device.hwVer == SDRPLAY_RSPduo_ID &&
        device.rspDuoMode != sdrplay_api_RspDuoMode_Single_Tuner && fsHz != 2e6))
        throw SDRplayException("invalid sample rate");
    sdrplay_api_ReasonForUpdateT reason = sdrplay_api_Update_None;
    if (device_params->devParams && fsHz != device_params->devParams->fsFreq.fsHz) {
        device_params->devParams->fsFreq.fsHz = fsHz;
        reason = (sdrplay_api_ReasonForUpdateT)(reason | sdrplay_api_Update_Dev_Fs);
    }
    sdrplay_api_DecimationT *decimation_params = &rx_channel_params->ctrlParams.decimation;
    if (decimationFactor != decimation_params->decimationFactor) {
        decimation_params->decimationFactor = decimationFactor;
        decimation_params->enable = decimationFactor > 1;
        reason = (sdrplay_api_ReasonForUpdateT)(reason | sdrplay_api_Update_Ctrl_Decimation);
    }
    if (run && reason != sdrplay_api_Update_None) {
        auto err = sdrplay_api_Update(device.dev, device.tuner, reason,
                                      sdrplay_api_Update_Ext1_None);
        if (err != sdrplay_api_Success)
            throw SDRplayException("sdrplay_api_Update(Dev_Fs|Ctrl_Decimation) failed");
    }

    this->samplerate = samplerate;

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setBandwidth(double samplerate)
{
    sdrplay_api_Bw_MHzT bwType = sdrplay_api_BW_Undefined;
    if      (samplerate <  300e3) { bwType = sdrplay_api_BW_0_200; }
    else if (samplerate <  600e3) { bwType = sdrplay_api_BW_0_300; }
    else if (samplerate < 1536e3) { bwType = sdrplay_api_BW_0_600; }
    else if (samplerate < 5000e3) { bwType = sdrplay_api_BW_1_536; }
    else if (samplerate < 6000e3) { bwType = sdrplay_api_BW_5_000; }
    else if (samplerate < 7000e3) { bwType = sdrplay_api_BW_6_000; }
    else if (samplerate < 8000e3) { bwType = sdrplay_api_BW_7_000; }
    else                          { bwType = sdrplay_api_BW_8_000; }
    if (bwType != rx_channel_params->tunerParams.bwType) {
        rx_channel_params->tunerParams.bwType = bwType;
        if (run) {
            auto err = sdrplay_api_Update(device.dev, device.tuner,
                                          sdrplay_api_Update_Tuner_BwType,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Tuner_BwType) failed");
        }
    }

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setFrequency(double frequency)
{
    constexpr double SDRPLAY_FREQ_MIN = 1e3;
    constexpr double SDRPLAY_FREQ_MAX = 2000e6;

    if (frequency < SDRPLAY_FREQ_MIN || frequency > SDRPLAY_FREQ_MAX)
        throw SDRplayException("invalid frequency");
    if (frequency != rx_channel_params->tunerParams.rfFreq.rfHz) {
        rx_channel_params->tunerParams.rfFreq.rfHz = frequency;
        if (run) {
            auto err = sdrplay_api_Update(device.dev, device.tuner,
                                          sdrplay_api_Update_Tuner_Frf,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Tuner_Frf) failed");
        }
    }

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setAntenna(const char* antenna)
{
    if (antenna == nullptr)
        return;

    if (device.hwVer == SDRPLAY_RSP2_ID) {
        sdrplay_api_Rsp2_AntennaSelectT antennaSel;
        sdrplay_api_Rsp2_AmPortSelectT amPortSel;
        if (strcmp(antenna, "Antenna A") == 0) {
            antennaSel = sdrplay_api_Rsp2_ANTENNA_A;
            amPortSel = sdrplay_api_Rsp2_AMPORT_2;
        } else if (strcmp(antenna, "Antenna B") == 0) {
            antennaSel = sdrplay_api_Rsp2_ANTENNA_B;
            amPortSel = sdrplay_api_Rsp2_AMPORT_2;
        } else if (strcmp(antenna, "Hi-Z") == 0) {
            antennaSel = sdrplay_api_Rsp2_ANTENNA_A;
            amPortSel = sdrplay_api_Rsp2_AMPORT_1;
        } else
            throw SDRplayException("invalid antenna");
        sdrplay_api_ReasonForUpdateT reason = sdrplay_api_Update_None;
        if (antennaSel != rx_channel_params->rsp2TunerParams.antennaSel) {
            rx_channel_params->rsp2TunerParams.antennaSel = antennaSel;
            reason = (sdrplay_api_ReasonForUpdateT)(reason | sdrplay_api_Update_Rsp2_AntennaControl);
        }
        if (amPortSel != rx_channel_params->rsp2TunerParams.amPortSel) {
            rx_channel_params->rsp2TunerParams.amPortSel = amPortSel;
            reason = (sdrplay_api_ReasonForUpdateT)(reason | sdrplay_api_Update_Rsp2_AmPortSelect);
        }
        if (run && reason != sdrplay_api_Update_None) {
            auto err = sdrplay_api_Update(device.dev, device.tuner, reason,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Rsp2_AntennaControl|Rsp2_AmPortSelect) failed");
        }
        return;
    }

    if (device.hwVer == SDRPLAY_RSPduo_ID) {
        sdrplay_api_TunerSelectT tuner;
        sdrplay_api_RspDuo_AmPortSelectT amPortSel;
        if (strcmp(antenna, "Tuner 1 50ohm") == 0) {
            tuner = sdrplay_api_Tuner_A;
            amPortSel = sdrplay_api_RspDuo_AMPORT_2;
        } else if (strcmp(antenna, "Tuner 2 50ohm") == 0) {
            tuner = sdrplay_api_Tuner_B;
            amPortSel = sdrplay_api_RspDuo_AMPORT_2;
        } else if (strcmp(antenna, "High Z") == 0) {
            tuner = sdrplay_api_Tuner_A;
            amPortSel = sdrplay_api_RspDuo_AMPORT_1;
        } else
            throw SDRplayException("invalid antenna");
        if (tuner != device.tuner) {
            if (device.rspDuoMode != sdrplay_api_RspDuoMode_Single_Tuner)
                throw SDRplayException("invalid antenna in master or slave mode");
            if (!run)
                device.tuner = tuner;
            if (run) {
                auto err = sdrplay_api_SwapRspDuoActiveTuner(device.dev,
                                                             &device.tuner,
                                                             amPortSel);
                if (err != sdrplay_api_Success)
                    throw SDRplayException("sdrplay_api_SwapRspDuoActiveTuner() failed");
            }
            rx_channel_params = device.tuner != sdrplay_api_Tuner_B ?
                                                device_params->rxChannelA :
                                                device_params->rxChannelB;
        }
        if (amPortSel != rx_channel_params->rspDuoTunerParams.tuner1AmPortSel) {
            rx_channel_params->rspDuoTunerParams.tuner1AmPortSel = amPortSel;
            if (run) {
                auto err = sdrplay_api_Update(device.dev, device.tuner,
                                              sdrplay_api_Update_RspDuo_AmPortSelect,
                                              sdrplay_api_Update_Ext1_None);
                if (err != sdrplay_api_Success)
                    throw SDRplayException("sdrplay_api_Update(RspDuo_AmPortSelect) failed");
            }
        }
        return;
    }

    if (device.hwVer == SDRPLAY_RSPdx_ID) {
        sdrplay_api_RspDx_AntennaSelectT antennaSel;
        if (strcmp(antenna, "Antenna A") == 0) {
            antennaSel = sdrplay_api_RspDx_ANTENNA_A;
        } else if (strcmp(antenna, "Antenna B") == 0) {
            antennaSel = sdrplay_api_RspDx_ANTENNA_B;
        } else if (strcmp(antenna, "Antenna C") == 0) {
            antennaSel = sdrplay_api_RspDx_ANTENNA_C;
        } else
            throw SDRplayException("invalid antenna");
        if (antennaSel != device_params->devParams->rspDxParams.antennaSel) {
            device_params->devParams->rspDxParams.antennaSel = antennaSel;
            if (run) {
                auto err = sdrplay_api_Update(device.dev, device.tuner,
                                              sdrplay_api_Update_None,
                                              sdrplay_api_Update_RspDx_AntennaControl);
                if (err != sdrplay_api_Success)
                    throw SDRplayException("sdrplay_api_Update(RspDx_AntennaControl) failed");
            }
        }

        return;
    }

    // can't change antenna for other models
    throw SDRplayException("invalid antenna");

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setIFGainReduction(int gRdB)
{
    if (gRdB == 0) {
        if (rx_channel_params->ctrlParams.agc.enable != sdrplay_api_AGC_CTRL_EN) {
            rx_channel_params->ctrlParams.agc.enable = sdrplay_api_AGC_CTRL_EN;
            if (run) {
                auto err = sdrplay_api_Update(device.dev, device.tuner,
                                              sdrplay_api_Update_Ctrl_Agc,
                                              sdrplay_api_Update_Ext1_None);
                if (err != sdrplay_api_Success)
                    throw SDRplayException("sdrplay_api_Update(Ctrl_Agc) failed");
            }
        }
    } else if (gRdB >= sdrplay_api_NORMAL_MIN_GR && gRdB <= MAX_BB_GR) {
        sdrplay_api_ReasonForUpdateT reason = sdrplay_api_Update_None;
        if (rx_channel_params->ctrlParams.agc.enable != sdrplay_api_AGC_DISABLE) {
            rx_channel_params->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
            reason = (sdrplay_api_ReasonForUpdateT)(reason | sdrplay_api_Update_Ctrl_Agc);
        }
        if (gRdB != rx_channel_params->tunerParams.gain.gRdB) {
            rx_channel_params->tunerParams.gain.gRdB = gRdB;
            reason = (sdrplay_api_ReasonForUpdateT)(reason | sdrplay_api_Update_Tuner_Gr);
        }
        if (run && reason != sdrplay_api_Update_None) {
            auto err = sdrplay_api_Update(device.dev, device.tuner, reason,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Ctrl_Agc|Tuner_Gr) failed");
        }
    } else
        throw SDRplayException("invalid IF gain reduction");

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setRFLnaState(unsigned char LNAstate)
{
    // checking for a valid RF LNA state is too complicated since it depends
    // on many factors like the device model, the frequency, etc
    // so I'll assume the user knows what they are doing
    if (LNAstate != rx_channel_params->tunerParams.gain.LNAstate) {
        rx_channel_params->tunerParams.gain.LNAstate = LNAstate;
        if (run) {
            auto err = sdrplay_api_Update(device.dev, device.tuner,
                                          sdrplay_api_Update_Tuner_Gr,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Tuner_Gr) failed");
        }
    }

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setIFType(int if_type)
{
    sdrplay_api_If_kHzT ifType = sdrplay_api_IF_Undefined;
    if      (if_type ==    0) { ifType = sdrplay_api_IF_Zero; }
    else if (if_type ==  450) { ifType = sdrplay_api_IF_0_450; }
    else if (if_type == 1620) { ifType = sdrplay_api_IF_1_620; }
    else if (if_type == 2048) { ifType = sdrplay_api_IF_2_048; }
    else
        throw SDRplayException("invalid IF type");
    if (ifType != rx_channel_params->tunerParams.ifType) {
        if (device.hwVer == SDRPLAY_RSPduo_ID &&
            device.rspDuoMode != sdrplay_api_RspDuoMode_Single_Tuner)
            throw SDRplayException("invalid IF type in master or slave mode");
        rx_channel_params->tunerParams.ifType = ifType;
        if (run) {
            auto err = sdrplay_api_Update(device.dev, device.tuner,
                                          sdrplay_api_Update_Tuner_IfType,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Tuner_IfType) failed");
        }
    }

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setDCOffset(bool enable)
{
    unsigned char DCenable = enable ? 1 : 0;
    if (DCenable != rx_channel_params->ctrlParams.dcOffset.DCenable) {
        rx_channel_params->ctrlParams.dcOffset.DCenable = DCenable;
        if (run) {
            auto err = sdrplay_api_Update(device.dev, device.tuner,
                                          sdrplay_api_Update_Ctrl_DCoffsetIQimbalance,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Ctrl_DCoffsetIQimbalance) failed");
        }
    }

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::setIQBalance(bool enable)
{
    unsigned char IQenable = enable ? 1 : 0;
    if (IQenable != rx_channel_params->ctrlParams.dcOffset.IQenable) {
        rx_channel_params->ctrlParams.dcOffset.DCenable = 1;
        rx_channel_params->ctrlParams.dcOffset.IQenable = IQenable;
        if (run) {
            auto err = sdrplay_api_Update(device.dev, device.tuner,
                                          sdrplay_api_Update_Ctrl_DCoffsetIQimbalance,
                                          sdrplay_api_Update_Ext1_None);
            if (err != sdrplay_api_Success)
                throw SDRplayException("sdrplay_api_Update(Ctrl_DCoffsetIQimbalance) failed");
        }
    }

    return;
}

template <typename T>
void SDRplaySourceMeasureDelay<T>::show_device_config() const
{
    std::cerr << std::endl;
    std::cerr << "# Device config:" << std::endl;
    for (auto rx_channel : { device_params->rxChannelA, device_params->rxChannelB }) {
        std::cerr << "RX channel=" << (rx_channel == device_params->rxChannelA ? "A" :
                   (rx_channel == device_params->rxChannelB ? "B" : "?")) << std::endl;
        if (!rx_channel)
            continue;
        sdrplay_api_TunerParamsT *tunerParams = &rx_channel->tunerParams;
        std::cerr << "    rfHz=" << tunerParams->rfFreq.rfHz << std::endl;
        std::cerr << "    bwType=" << tunerParams->bwType << std::endl;
        std::cerr << "    ifType=" << tunerParams->ifType << std::endl;
        sdrplay_api_DecimationT *decimation = &rx_channel->ctrlParams.decimation;
        std::cerr << "    decimationFactor=" << static_cast<unsigned int>(decimation->decimationFactor) << std::endl;
        std::cerr << "    decimation.enable=" << static_cast<unsigned int>(decimation->enable) << std::endl;
        std::cerr << "    gain.gRdB=" << tunerParams->gain.gRdB << std::endl;
        std::cerr << "    gain.LNAstate=" << static_cast<unsigned int>(tunerParams->gain.LNAstate) << std::endl;
        sdrplay_api_AgcT *agc = &rx_channel->ctrlParams.agc;
        std::cerr << "    agc.enable=" << agc->enable << std::endl;
        std::cerr << "    agc.setPoint_dBfs=" << agc->setPoint_dBfs << std::endl;
        std::cerr << "    agc.attack_ms=" << agc->attack_ms << std::endl;
        std::cerr << "    agc.decay_ms=" << agc->decay_ms << std::endl;
        std::cerr << "    agc.decay_delay_ms=" << agc->decay_delay_ms << std::endl;
        std::cerr << "    agc.decay_threshold_dB=" << agc->decay_threshold_dB << std::endl;
        std::cerr << "    agc.syncUpdate=" << agc->syncUpdate << std::endl;
        std::cerr << "    dcOffset.DCenable=" << static_cast<unsigned int>(rx_channel->ctrlParams.dcOffset.DCenable) << std::endl;
        std::cerr << "    dcOffsetTuner.dcCal=" << static_cast<unsigned int>(tunerParams->dcOffsetTuner.dcCal) << std::endl;
        std::cerr << "    dcOffsetTuner.speedUp=" << static_cast<unsigned int>(tunerParams->dcOffsetTuner.speedUp) << std::endl;
        std::cerr << "    dcOffsetTuner.trackTime=" << tunerParams->dcOffsetTuner.trackTime << std::endl;
        std::cerr << "    dcOffset.IQenable=" << static_cast<unsigned int>(rx_channel->ctrlParams.dcOffset.IQenable) << std::endl;
    }
    std::cerr << std::endl;
    if (device_params->devParams) {
        std::cerr << "fsHz=" << device_params->devParams->fsFreq.fsHz << std::endl;
        std::cerr << "ppm=" << device_params->devParams->ppm << std::endl;
    }
    std::cerr << std::endl;
    return;
}

namespace Csdrx {
    template class SDRplaySourceMeasureDelay<Csdr::complex<short>>;
    template class SDRplaySourceMeasureDelay<Csdr::complex<float>>;
}
