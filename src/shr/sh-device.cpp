#include "sh-device.hpp"

using namespace std;

////////////////////
/// SHDevice
SHDevice::SHDevice()
{

}

SHDevice::~SHDevice()
{
    if (m_handle != -1)
    {
        bbAbort(m_handle);
        bbCloseDevice(m_handle);
    }
}

void SHDevice::open()
{
    bbStatus status;
    status = bbOpenDevice(&m_handle);
    if(status != bbNoError) {
        throw std::runtime_error(string("Unable to open device. Error: ") + bbGetErrorString(status));
    } else {
        //cout << "Device opened successfully" << endl;
    }
}

void SHDevice::configureCenterSpan(double center, double span)
{
    checkRetCode(
        bbConfigureCenterSpan(m_handle, center, span),
        "bbConfigureCenterSpan"
    );
    m_bandwidth = span;
}

void SHDevice::configureLevel(double reference, double attenuator)
{
    checkRetCode(
        bbConfigureLevel(m_handle, reference, attenuator),
        "bbConfigureLevel"
    );
}

void SHDevice::configureGain(int gain)
{
    checkRetCode(
        bbConfigureGain(m_handle, gain),
        "bbConfigureGain"
    );
}

void SHDevice::runRealTime()
{
    checkRetCode(
        bbConfigureIO(m_handle, BB_PORT1_INT_REF_OUT, BB_PORT2_IN_TRIGGER_RISING_EDGE),
        "bbConfigureIO"
    );

    checkRetCode(
        bbConfigureIQ(m_handle, dowsampleFactor(m_bandwidth), m_bandwidth),
        "bbConfigureIQ"
    );
    checkRetCode(
        bbInitiate(m_handle, BB_STREAMING, BB_STREAM_IQ),
        "bbInitiate"
    );
}

size_t SHDevice::monitorGetNotReaded() const
{
    return m_notReaded;
}

int SHDevice::handle() const
{
    return m_handle;
}

unique_ptr<StreamPacket> SHDevice::getPacket(size_t size)
{
    unique_ptr<StreamPacket> psp(new StreamPacket(size));
    bbGetIQ(m_handle, &psp->packet);
    m_notReaded = psp->packet.dataRemaining;
    return psp;
}

void SHDevice::checkRetCode(bbStatus ret, const std::string& message)
{
    if (ret != bbNoError)
    {
        throw std::runtime_error(message + "; error code " + std::to_string((int) ret) + ": " + bbGetErrorString(ret));
    }
}

int SHDevice::dowsampleFactor(double bandwidth)
{
    if (bandwidth > 17.8e6)
        return 1;
    if (bandwidth > 8e6)
        return 2;
    if (bandwidth > 3.75e6)
        return 4;
    if (bandwidth > 2e6)
        return 8;
    if (bandwidth > 1e6)
        return 16;
    if (bandwidth > 0.5e6)
        return 32;
    if (bandwidth > 0.125e6)
        return 64;
    if (bandwidth > 140e3)
        return 128;
    if (bandwidth > 65e3)
        return 256;
    if (bandwidth > 30e3)
        return 512;
    if (bandwidth > 15e3)
        return 1024;
    if (bandwidth > 8e3)
        return 2048;
    if (bandwidth > 4e3)
        return 4096;
    return 8192;
}
