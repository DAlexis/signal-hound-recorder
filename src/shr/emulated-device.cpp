#include "emulated-device.hpp"

#include <iostream>
#include <cstring>

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

    if (interval < m_period) // Block not ready
        return std::unique_ptr<StreamPacket>();

    m_notReadSamples += interval / m_period * m_size;
    size_t toRead = std::min(m_notReadSamples, size);

    if (toRead == 0)
        return std::unique_ptr<StreamPacket>();

    m_lastTime = now;

    //static int i=0;
    //cout << i++<< " to read: " << toRead << " interval " << interval << " sleep " << m_sleepOnReceive << endl;

    std::unique_ptr<StreamPacket> pkt(new StreamPacket(toRead));
    //memset(pkt->data.data(), 0, sizeof(pkt->data[0]) * toRead * 2);
    float *d = pkt->data.data();

    for (size_t i=0; i<toRead; i++)
    {
        d[2*i] = m_sampleIndex;
        d[2*i+1] = m_sampleIndex;
        m_sampleIndex++;
    }
    m_notReadSamples -= toRead;

    if (m_sleepOnReceive != 0.0)
        std::this_thread::sleep_for(std::chrono::microseconds(size_t(m_sleepOnReceive*1e6)));
    return pkt;
}

size_t DeviceEmulator::monitorGetNotReaded() const
{
    return m_notReadSamples;
}
