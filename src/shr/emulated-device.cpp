#include "emulated-device.hpp"

using namespace std;

////////////////////
/// DeviceEmulator
DeviceEmulator::DeviceEmulator(double period, size_t size, double sleepOnReceive) :
    m_period(period), m_size(size), m_sleepOnReceive(sleepOnReceive)
{
}

std::unique_ptr<StreamPacket> DeviceEmulator::getPacket(size_t size)
{
    auto now = std::chrono::steady_clock::now();
    double interval = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastTime).count() * 1e-6;
    m_notReadSamples += interval / m_period * m_size;
    size_t toRead = std::min(m_notReadSamples, size);

    if (toRead == 0)
        return std::unique_ptr<StreamPacket>();

    m_lastTime = now;

    //static int i=0;
    //cout << i++<< " to read: " << toRead << " interval " << interval << endl;

    std::unique_ptr<StreamPacket> pkt(new StreamPacket(toRead));
    for (size_t i=0; i<toRead; i++)
    {
        pkt->data[2*i] = m_sampleIndex;
        pkt->data[2*i+1] = m_sampleIndex;
        m_sampleIndex++;
    }

    m_notReadSamples -= toRead;
    std::this_thread::sleep_for(std::chrono::microseconds(size_t(m_sleepOnReceive*1e6)));
    return pkt;
}

size_t DeviceEmulator::monitorGetNotReaded() const
{
    return m_notReadSamples;
}
