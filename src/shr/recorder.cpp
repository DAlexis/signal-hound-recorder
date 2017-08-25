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
	double interval = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastTime).count();
	m_notReadSamples += interval / m_period * m_size;
	size_t toRead = std::min(m_notReadSamples, size);

	std::unique_ptr<StreamPacket> pkt(new StreamPacket(toRead));
	for (size_t i=0; i<toRead; i++)
		pkt->data[i] = i;
	return pkt;
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
	std::unique_lock<std::mutex> lck(m_queueMutex);
	//m_queue.push(std::move(sp));
	//m_queueCv.notify_all();
}

void SinkBase::run()
{
	m_consumerThread = std::thread([this](){ consumerLoop(); });
}

void SinkBase::stop()
{
	m_needStop = true;
	m_queueCv.notify_all();
}

void SinkBase::join()
{
	if (m_consumerThread.joinable())
		m_consumerThread.join();
}

void SinkBase::consumerLoop()
{
	cout << "Running consumer loop" << endl;
	while (!m_needStop)
	{
		std::unique_lock<std::mutex> lck(m_queueMutex);
		if (m_queue.empty())
			m_queueCv.wait(lck, [this](){ return m_needStop || !m_queue.empty(); });

		std::unique_ptr<StreamPacket> psp = std::move(m_queue.front());
		m_queue.pop();
		consume(std::move(psp));
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
	std::unique_ptr<StreamPacket> p(m_provider.getPacket(blockSize));
	if (p->packet.iqCount != 0)
		m_sink.enqueue(std::move(p));
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
    : m_dataFile(filename, ios::out),
      m_indexFile(filename, ios::out)
{
}

void FileSink::consume(std::unique_ptr<StreamPacket> sp)
{
	cout << "FileSink::consume" << endl;
	HeaderSerialized hs(*sp, m_offset);
	m_indexFile.write(reinterpret_cast<const char*>(&hs), sizeof(hs));
	m_dataFile.write(reinterpret_cast<const char*>(sp->data.data()), sizeof(sp->data[0]) * 2 * sp->packet.iqCount);
	//m_fileStream.write();
	m_offset += sp->packet.iqCount;
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
	DeviceEmulator *emulator = new DeviceEmulator(1e-3, 1e3);
	m_streamProvider.reset(emulator);
	m_connector.reset(new Connector(*m_streamProvider, *m_fileSink));
}

void Recorder::stop()
{
	m_stopNow = true;
	m_fileSink->stop();
}

void Recorder::run()
{
	if (!m_needRun)
		return;

	m_fileSink.reset(new FileSink(m_p["Streaming"].get<std::string>("file-prefix")));

	if (m_p["Streaming"].get<bool>("simulate"))
		configureSimulator();
	else
		configureDevice();

	size_t bs = m_p["Streaming"].get<size_t>("max-block-size");
	while (!m_stopNow)
	{
		m_connector->tick(bs);
		//cout << "[received] block with size " << bs << endl;
	}
	//m_fileSink->join();
}

