#include "recorder.hpp"
#include "bb_api.h"

#include <iostream>

using namespace std;

////////////////////
/// StreamPacket

StreamPacket::StreamPacket(size_t size)
{
	data.resize(size*2, 0.0);

	packet.iqData = data.data();
	packet.iqCount = size;
	packet.triggers = nullptr;
	packet.triggerCount = 0;
	packet.purge = BB_FALSE;
	packet.dataRemaining = 0;
	packet.sampleLoss = 0;
	packet.sec = 0;
	packet.nano = 0;
}

////////////////////
/// DeviceEmulator
DeviceEmulator::DeviceEmulator(double period, size_t size) :
    m_period(period), m_size(size)
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
	return pkt;
}

size_t DeviceEmulator::monitorGetNotReaded() const
{
    return m_notReadSamples;
}

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
		cerr << "Unable to open device" << endl;
		cerr << bbGetErrorString(status) << endl;
		return;
	} else {
		cerr << "Device opened successfully" << endl;
	}
}

void SHDevice::configureCenterSpan(double center, double span)
{
	bbConfigureCenterSpan(m_handle, center, span);
}

void SHDevice::configureLevel(double reference, double attenuator)
{
	bbConfigureLevel(m_handle, reference, attenuator);
}

void SHDevice::configureGain(int gain)
{
	bbConfigureGain(m_handle, gain);
}

void SHDevice::runRealTime()
{
	bbStatus ret = bbNoError;
	ret = bbConfigureIO(m_handle, BB_PORT1_INT_REF_OUT, BB_PORT2_IN_TRIGGER_RISING_EDGE);
	checkRetCode(ret, "bbConfigureIO");
	ret = bbConfigureIQ(m_handle, 4, m_bandwidth);
	checkRetCode(ret, "bbConfigureIQ");
	ret = bbInitiate(m_handle, BB_STREAMING, BB_STREAM_IQ);
	checkRetCode(ret, "bbInitiate");
}

size_t SHDevice::monitorGetNotReaded() const
{
    return 0;
}

unique_ptr<StreamPacket> SHDevice::getPacket(size_t size)
{
	unique_ptr<StreamPacket> psp(new StreamPacket(size));
	bbGetIQ(m_handle, &psp->packet);
	return psp;
}

void SHDevice::checkRetCode(bbStatus ret, const std::string& message)
{
	if (ret != bbNoError)
	{
		throw std::runtime_error(message + "; error code " + std::to_string((int) ret));
	}
}

///////////////////////
/// SinkBase

void SinkBase::enqueue(std::unique_ptr<StreamPacket> sp)
{
    //cout << "SinkBase::enqueue" << endl;
	std::unique_lock<std::mutex> lck(m_queueMutex);
    m_queue.push(std::move(sp));
    m_queueCv.notify_all();
}

void SinkBase::run()
{
	m_consumerThread = std::thread([this](){ consumerLoop(); });
}

void SinkBase::stop()
{
	m_needStop = true;
	m_queueCv.notify_all();

    if (m_consumerThread.joinable())
        m_consumerThread.join();
}

void SinkBase::consumerLoop()
{
	cout << "Running consumer loop" << endl;
    std::queue<std::unique_ptr<StreamPacket>> toWrite;
	while (!m_needStop)
	{
		std::unique_lock<std::mutex> lck(m_queueMutex);
		if (m_queue.empty())
			m_queueCv.wait(lck, [this](){ return m_needStop || !m_queue.empty(); });

        // Moving all content from queue and quickly unlocking it
        toWrite.swap(m_queue);
        lck.unlock();

        //std::this_thread::sleep_for(std::chrono::seconds(3));
        while (!toWrite.empty())
        {
            std::unique_ptr<StreamPacket> psp = std::move(toWrite.front());
            toWrite.pop();
            consume(std::move(psp));
        }
	}
}

///////////////////////
/// Connector

Connector::Connector(IStreamProvider& provider, SinkBase& sink) :
    m_provider(provider),
    m_sink(sink)
{

}

void Connector::tick(size_t blockSize)
{
    std::unique_ptr<StreamPacket> p = m_provider.getPacket(blockSize);
    if (p.get() != nullptr && p->packet.iqCount != 0)
    {
		m_sink.enqueue(std::move(p));
    }
}

HeaderSerialized::HeaderSerialized(StreamPacket& pkt, size_t offset, size_t triggersOffset)
{
	this->offset = offset;
	iqCount = pkt.packet.iqCount;
	triggersOffset = triggersOffset;
	triggerCount = pkt.packet.triggerCount;
	purge = pkt.packet.purge;
	dataRemaining = pkt.packet.dataRemaining;
	sampleLoss = pkt.packet.sampleLoss;
	sec = pkt.packet.sec;
	nano = pkt.packet.nano;
}

///////////////////////
/// FileSink

FileSink::FileSink(const std::string& filename)
    : m_dataFile(filename + "_data.bin", ios::out),
      m_indexFile(filename + "_index.bin", ios::out)
{
}

size_t FileSink::currentSize() const
{
    return m_offset;
}

void FileSink::consume(std::unique_ptr<StreamPacket> sp)
{
    //cout << "FileSink::consume" << endl;
	HeaderSerialized hs(*sp, m_offset);
	m_indexFile.write(reinterpret_cast<const char*>(&hs), sizeof(hs));
	m_dataFile.write(reinterpret_cast<const char*>(sp->data.data()), sizeof(sp->data[0]) * 2 * sp->packet.iqCount);
	m_offset += sp->packet.iqCount;
}


///////////////////////
/// SHRecorder
Monitor::Monitor(const IStreamProvider* provider, const SinkBase* sink, const Connector* connector) :
    m_provide(provider),
    m_sink(sink),
    m_connector(connector)
{

}

void Monitor::run()
{
    m_thread = std::thread([this](){ mainLoop(); });
}

void Monitor::stop()
{
    m_needStop = true;
    if (m_thread.joinable())
        m_thread.join();
}

void Monitor::mainLoop()
{
    cout << "Monitor running" << endl;
    while (!m_needStop)
    {
        if (waitOrStop(1000))
            return;
        printStatus();
    }
}

void Monitor::printStatus()
{
    cout << "[Recording] file size: " << m_sink->currentSize()
         << "; not readed samples: " << m_provide->monitorGetNotReaded() << endl;
}

bool Monitor::waitOrStop(int milliseconds)
{
    int delay = 0;
    auto begin = chrono::steady_clock::now();
    while (delay  < milliseconds && !m_needStop)
    {
        delay = chrono::duration_cast<std::chrono::milliseconds>(
            chrono::steady_clock::now() - begin
        ).count();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return m_needStop;
}

///////////////////////
/// SHRecorder

Recorder::Recorder()
{
	cic::PreconfiguredOperations::addGeneralOptions(m_p);
}

void Recorder::readConfig(int argc, char** argv)
{
	try {
		m_needRun = cic::PreconfiguredOperations::quickReadConfiguration(
		    m_p,
		    {
		        "/etc/shr.conf",
		        "~/.local/etc/shr.conf",
		        "shr.conf"
		    },
		    argc,
		    argv
		);
	} catch (std::exception &ex) {
		m_needRun = false;
		cerr << "Error while reading configuration: " << ex.what() << endl;
	}
}

void Recorder::configureDevice()
{
	SHDevice *device = new SHDevice();
	m_streamProvider.reset(device);

	device->configureCenterSpan(
	    m_p["Band"].get<double>("freq-center"),
	    m_p["Band"].get<double>("bandwidth")
	);
	device->configureLevel(
	    m_p["Level"].get<double>("reference"),
	    m_p["Level"].get<double>("attenuator")
	);
	device->configureGain();
	device->runRealTime();
	m_connector.reset(new Connector(*m_streamProvider, *m_fileSink));
}

void Recorder::configureSimulator()
{
	cout << "Running with device emulation" << endl;
    DeviceEmulator *emulator = new DeviceEmulator(
        m_p["Device simulation"].get<double>("test-block-period"),
        m_p["Device simulation"].get<size_t>("test-block-size")
    );

	m_streamProvider.reset(emulator);
	m_connector.reset(new Connector(*m_streamProvider, *m_fileSink));
}

void Recorder::stop()
{
	m_stopNow = true;
}

void Recorder::run()
{
	if (!m_needRun)
		return;

	m_fileSink.reset(new FileSink(m_p["Streaming"].get<std::string>("file-prefix")));

    m_fileSink->run();

    if (m_p["Device simulation"].get<bool>("simulate"))
		configureSimulator();
	else
		configureDevice();

    m_monitor.reset(new Monitor(m_streamProvider.get(), m_fileSink.get(), m_connector.get()));
    m_monitor->run();
	size_t bs = m_p["Streaming"].get<size_t>("max-block-size");
	while (!m_stopNow)
	{
		m_connector->tick(bs);
	}
    // Now stopping all code
    m_fileSink->stop();
    m_monitor->stop();
}

