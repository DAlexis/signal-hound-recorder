#ifndef SHDEVICE_HPP
#define SHDEVICE_HPP

#include "communication.hpp"

#include <memory>

class SHDevice : public IStreamProvider
{
public:
    SHDevice();
    ~SHDevice();
    void open();
    void configureCenterSpan(double center, double span);
    void configureLevel(double reference = defaultReference, double attenuator = defaultAttenuator);
    void configureGain(int gain = BB_AUTO_GAIN);
    void runRealTime();

    int handle() const;

    size_t monitorGetNotReaded() const override;

    std::unique_ptr<StreamPacket> getPacket(size_t size) override;

    constexpr static double minFreq = 9e3;
    constexpr static double maxBandwidth = 27e6;
    constexpr static double centerForMinFreqMaxBW = minFreq + maxBandwidth / 2.0;
    constexpr static double defaultReference = -20;
    constexpr static double defaultAttenuator = BB_AUTO_ATTEN;
private:
    void checkRetCode(bbStatus ret, const std::string& message = "");

    static int dowsampleFactor(double bandwidth);

    int m_handle = -1;
    double m_bandwidth = 27e6;
};

#endif // SHDEVICE_HPP
