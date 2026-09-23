/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AHAL_StreamAlsaMonoPipe"
#include <algorithm>
#include <chrono>
#include <Log.h>

#include <audio_utils/clock.h>
#include <media/AidlConversionCppNdk.h>

#include "core-impl/StreamAlsaMonoPipe.h"

using aidl::android::hardware::audio::common::getChannelCount;

namespace aidl::android::hardware::audio::core {

StreamAlsaMonoPipe::StreamAlsaMonoPipe(StreamContext* context, const Metadata& metadata,
                                       int readWriteRetries)
    : StreamAlsaBase(context, metadata, readWriteRetries) {}

StreamAlsaMonoPipe::~StreamAlsaMonoPipe() {
    cleanupWorker();
}

::android::NBAIO_Format StreamAlsaMonoPipe::getPipeFormat() const {
    const audio_format_t audioFormat = VALUE_OR_FATAL(
            aidl2legacy_AudioFormatDescription_audio_format_t(getContext().getFormat()));
    const int channelCount = getChannelCount(getContext().getChannelLayout());
    return ::android::Format_from_SR_C(getContext().getSampleRate(), channelCount, audioFormat);
}

::android::sp<::android::MonoPipe> StreamAlsaMonoPipe::makeSink(bool writeCanBlock) {
    const ::android::NBAIO_Format format = getPipeFormat();
    auto sink = ::android::sp<::android::MonoPipe>::make(mBufferSizeFrames, format, writeCanBlock);
    const ::android::NBAIO_Format offers[1] = {format};
    size_t numCounterOffers = 0;
    ssize_t index = sink->negotiate(offers, 1, nullptr, numCounterOffers);
    LOG_IF(FATAL, index != 0) << __func__ << ": Negotiation for the sink failed, index = " << index;
    return sink;
}

::android::sp<::android::MonoPipeReader> StreamAlsaMonoPipe::makeSource(::android::MonoPipe* pipe) {
    const ::android::NBAIO_Format format = getPipeFormat();
    const ::android::NBAIO_Format offers[1] = {format};
    auto source = ::android::sp<::android::MonoPipeReader>::make(pipe);
    size_t numCounterOffers = 0;
    ssize_t index = source->negotiate(offers, 1, nullptr, numCounterOffers);
    LOG_IF(FATAL, index != 0) << __func__
                              << ": Negotiation for the source failed, index = " << index;
    return source;
}

::android::status_t StreamAlsaMonoPipe::standby() {
    teardownIo();
    return ::android::OK;
}

::android::status_t StreamAlsaMonoPipe::start() {
    if (!mAlsaDeviceProxies.empty()) {
        // This is a resume after a pause.
        return ::android::OK;
    }
    decltype(mAlsaDeviceProxies) alsaDeviceProxies;
    decltype(mSources) sources;
    decltype(mSinks) sinks;
    if (::android::status_t status = openDeviceProxies(&alsaDeviceProxies);
        status != ::android::OK) {
        return status;
    }
    for (size_t i = 0; i < alsaDeviceProxies.size(); ++i) {
        auto sink = makeSink(mIsInput);  // Do not block the writer when it is on our thread.
        if (sink != nullptr) {
            sinks.push_back(sink);
        } else {
            return ::android::NO_INIT;
        }
        if (auto source = makeSource(sink.get()); source != nullptr) {
            sources.push_back(source);
        } else {
            return ::android::NO_INIT;
        }
    }
    mAlsaDeviceProxies = std::move(alsaDeviceProxies);
    mSources = std::move(sources);
    mSinks = std::move(sinks);
    mClockFrames = 0;
    mClockError = false;
    mIoThreadIsRunning = true;
    for (size_t i = 0; i < mAlsaDeviceProxies.size(); ++i) {
        mIoThreads.emplace_back(
                mIsInput ? &StreamAlsaMonoPipe::inputIoThread : &StreamAlsaMonoPipe::outputIoThread,
                this, i);
    }
    return ::android::OK;
}

bool StreamAlsaMonoPipe::waitForOutputClock() {
    if (mAlsaDeviceProxies.empty()) return false;
    const auto& config = mAlsaDeviceProxies[0].get()->alsa_config;
    if (config.rate != 48000 || config.channels != 2) return false;
    const uint64_t threshold = std::max<uint64_t>(config.start_threshold,
            uint64_t(config.period_size) * config.period_count) + config.period_size;
    for (int i = 0; i < 1000 && !mClockError; ++i) {
        if (mClockFrames >= threshold) return true;
        usleep(1000);
    }
    return false;
}

::android::status_t StreamAlsaMonoPipe::transfer(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount, int32_t* latencyMs) {
    if (mAlsaDeviceProxies.empty()) {
        LOG(FATAL) << __func__ << ": no opened devices";
        return ::android::NO_INIT;
    }
    const size_t bytesToTransfer = frameCount * mFrameSizeBytes;
    unsigned maxLatency = 0;
    if (mIsInput) {
        const size_t i = 0;  // For the input case, only support a single device.
        LOG(VERBOSE) << __func__ << ": reading from sink " << i;
        size_t framesRead = 0;
        // MonoPipeReader::read() is nonblocking. Pace the caller until a complete
        // capture period is available so a USB scheduling race does not become silence.
        const int64_t periodUs =
                static_cast<int64_t>(frameCount) * MICROS_PER_SECOND / mSampleRate;
        const auto waitLimit =
                std::chrono::microseconds(std::max<int64_t>(250'000, periodUs * 4));
        const auto deadline = std::chrono::steady_clock::now() + waitLimit;
        while (framesRead < frameCount && mIoThreadIsRunning) {
            ssize_t readResult = mSources[i]->read(
                    static_cast<char*>(buffer) + framesRead * mFrameSizeBytes,
                    frameCount - framesRead);
            LOG_IF(FATAL, readResult < 0) << "Error reading from the pipe: " << readResult;
            framesRead += static_cast<size_t>(readResult);
            if (framesRead == frameCount || std::chrono::steady_clock::now() >= deadline) break;
            usleep(500);
        }
        if (size_t framesMissing = frameCount - framesRead; framesMissing > 0) {
            LOG(WARNING) << __func__ << ": incomplete data received, inserting " << framesMissing
                         << " frames of silence";
            memset(static_cast<char*>(buffer) + framesRead * mFrameSizeBytes, 0,
                   framesMissing * mFrameSizeBytes);
        }
        maxLatency = proxy_get_latency(mAlsaDeviceProxies[i].get());
    } else {
        alsa::applyGain(buffer, mGain, bytesToTransfer, mConfig.value().format, mConfig->channels);
        for (size_t i = 0; i < mAlsaDeviceProxies.size(); ++i) {
            LOG(VERBOSE) << __func__ << ": writing into sink " << i;
            ssize_t framesWritten = mSinks[i]->write(buffer, frameCount);
            LOG_IF(FATAL, framesWritten < 0) << "Error writing into the pipe: " << framesWritten;
            if (ssize_t framesLost = static_cast<ssize_t>(frameCount) - framesWritten;
                framesLost > 0) {
                LOG(WARNING) << __func__ << ": sink " << i << " incomplete data sent, dropping "
                             << framesLost << " frames";
            }
            maxLatency = std::max(maxLatency, proxy_get_latency(mAlsaDeviceProxies[i].get()));
        }
    }
    *actualFrameCount = frameCount;
    maxLatency = std::min(maxLatency, static_cast<unsigned>(std::numeric_limits<int32_t>::max()));
    *latencyMs = maxLatency;
    return ::android::OK;
}

void StreamAlsaMonoPipe::shutdown() {
    teardownIo();
}

void StreamAlsaMonoPipe::inputIoThread(size_t idx) {
#if defined(__ANDROID__)
    setWorkerThreadPriority(pthread_gettid_np(pthread_self()));
    const std::string threadName = (std::string("in_") + std::to_string(idx)).substr(0, 15);
    pthread_setname_np(pthread_self(), threadName.c_str());
#endif
    const size_t bufferSize = mBufferSizeFrames * mFrameSizeBytes;
    std::vector<char> buffer(bufferSize);
    while (mIoThreadIsRunning) {
        if (int ret = proxy_read_with_retries(mAlsaDeviceProxies[idx].get(), &buffer[0], bufferSize,
                                              mReadWriteRetries);
            ret == 0) {
            size_t bufferFramesWritten = 0;
            while (bufferFramesWritten < mBufferSizeFrames) {
                if (!mIoThreadIsRunning) return;
                ssize_t framesWrittenOrError =
                        mSinks[idx]->write(&buffer[0], mBufferSizeFrames - bufferFramesWritten);
                if (framesWrittenOrError >= 0) {
                    bufferFramesWritten += framesWrittenOrError;
                } else {
                    LOG(WARNING) << __func__ << "[" << idx
                                 << "]: Error while writing into the pipe: "
                                 << framesWrittenOrError;
                }
            }
        } else {
            // Errors when the stream is being stopped are expected.
            LOG_IF(WARNING, mIoThreadIsRunning)
                    << __func__ << "[" << idx << "]: Error reading from ALSA: " << ret;
        }
    }
}

void StreamAlsaMonoPipe::outputIoThread(size_t idx) {
#if defined(__ANDROID__)
    setWorkerThreadPriority(pthread_gettid_np(pthread_self()));
    const std::string threadName = (std::string("out_") + std::to_string(idx)).substr(0, 15);
    pthread_setname_np(pthread_self(), threadName.c_str());
#endif
    const size_t bufferSize = mBufferSizeFrames * mFrameSizeBytes;
    std::vector<char> buffer(bufferSize);
    while (mIoThreadIsRunning) {
        ssize_t framesReadOrError = mSources[idx]->read(&buffer[0], mBufferSizeFrames);
        if (mKeepOutputClock) {
            const size_t readFrames = std::max<ssize_t>(0, framesReadOrError);
            if (silenceOutput()) std::fill(buffer.begin(), buffer.end(), 0);
            else std::fill(buffer.begin() + readFrames * mFrameSizeBytes, buffer.end(), 0);
            framesReadOrError = mBufferSizeFrames;
        }
        if (framesReadOrError > 0) {
            int ret = proxy_write_with_retries(mAlsaDeviceProxies[idx].get(), &buffer[0],
                                               framesReadOrError * mFrameSizeBytes,
                                               mReadWriteRetries);
            if (mKeepOutputClock && idx == 0) {
                if (ret == 0) mClockFrames += framesReadOrError;
                else if (mIoThreadIsRunning) {
                    mClockError = true;
                    outputClockFailed();
                    usleep(10000);  // Do not spin on a disconnected/failed PCM.
                }
            }
            // Errors when the stream is being stopped are expected.
            LOG_IF(WARNING, ret != 0 && mIoThreadIsRunning)
                    << __func__ << "[" << idx << "]: Error writing into ALSA: " << ret;
        } else if (framesReadOrError == 0) {
            // MonoPipeReader does not have a blocking read, while use of std::condition_variable
            // requires use of a mutex. For now, just do a 1ms sleep. Consider using a different
            // pipe / ring buffer mechanism.
            if (mIoThreadIsRunning) usleep(1000);
        } else {
            LOG(WARNING) << __func__ << "[" << idx
                         << "]: Error while reading from the pipe: " << framesReadOrError;
        }
    }
}

void StreamAlsaMonoPipe::teardownIo() {
    mIoThreadIsRunning = false;
    if (mIsInput) {
        LOG(DEBUG) << __func__ << ": shutting down pipes";
        for (auto& sink : mSinks) {
            sink->shutdown(true);
        }
    }
    LOG(DEBUG) << __func__ << ": stopping PCM streams";
    for (const auto& proxy : mAlsaDeviceProxies) {
        proxy_stop(proxy.get());
    }
    LOG(DEBUG) << __func__ << ": joining threads";
    for (auto& thread : mIoThreads) {
        if (thread.joinable()) thread.join();
    }
    mIoThreads.clear();
    LOG(DEBUG) << __func__ << ": closing PCM devices";
    mAlsaDeviceProxies.clear();
    mSources.clear();
    mSinks.clear();
}

}  // namespace aidl::android::hardware::audio::core
