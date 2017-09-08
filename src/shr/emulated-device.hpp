#ifndef EMULATEDDEVICE_HPP
#define EMULATEDDEVICE_HPP

#include "communication.hpp"

class DeviceEmulator : public IStreamProvider
{
public:
    DeviceEmulator(double period = 1e-3, size_t size = 1e3, double sleepOnReceive = 0.0);
    std::unique_ptr<StreamPacket> getPacket(size_t size) override;
    size_t monitorGetNotReaded() const override;

private:
    size_t m_notReadSamples = 0;
    std::chrono::steady_clock::time_point m_lastTime = std::chrono::steady_clock::now();
    double m_period;
    size_t m_size;
    double m_sleepOnReceive;
    size_t m_sampleIndex = 0;
};

#endif // EMULATEDDEVICE_HPP
