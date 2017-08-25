#ifndef RECORDER_HPP
#define RECORDER_HPP

#include "cic.hpp"
#include "bb_api.h"

#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

struct StreamPacket
{
	StreamPacket(size_t size);
	bbIQPacket packet;
	std::vector<float> data;
};

class IStreamProvider
{
public:
	virtual ~IStreamProvider() { }
	virtual std::unique_ptr<StreamPacket> getPacket(size_t size) = 0;
};

class DeviceEmulator : public IStreamProvider
{
public:
	DeviceEmulator(double period = 1e-3, size_t size = 1e3);
	std::unique_ptr<StreamPacket> getPacket(size_t size) override;

private:
	size_t m_notReadSamples = 0;
	std::chrono::steady_clock::time_point m_lastTime = std::chrono::steady_clock::now();
	double m_period;
	size_t m_size;
};

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

	std::unique_ptr<StreamPacket> getPacket(size_t size) override;

	constexpr static double minFreq = 9e3;
	constexpr static double maxBandwidth = 27e6;
	constexpr static double centerForMinFreqMaxBW = minFreq + maxBandwidth / 2.0;
	constexpr static double defaultReference = -20;
	constexpr static double defaultAttenuator = BB_AUTO_ATTEN;
private:
	void checkRetCode(bbStatus ret, const std::string& message = "");

	int m_handle = -1;
	double m_bandwidth = 27e6;
};

class SinkBase
{
public:
	void enqueue(std::unique_ptr<StreamPacket> sp);

	void run();
	void stop();
	void join();

protected:
	virtual void consume(std::unique_ptr<StreamPacket> sp) = 0;

	void consumerLoop();

	std::queue<std::unique_ptr<StreamPacket>> m_queue;
	std::mutex m_queueMutex;
	std::condition_variable m_queueCv;
	std::thread m_consumerThread;
	bool m_needStop = false;
};

class Connector
{
public:
	Connector(IStreamProvider& provider, SinkBase& sink);

	void tick(size_t blockSize);

private:
	IStreamProvider& m_provider;
	SinkBase& m_sink;
};

#pragma pack (push, 0)
struct HeaderSerialized {
	HeaderSerialized(StreamPacket& pkt, size_t offset, size_t triggersOffset = 0);

	size_t offset;
	int iqCount;
	size_t triggersOffset;
	int triggerCount;
	int purge;
	int dataRemaining;
	int sampleLoss;
	int sec;
	int nano;
};
#pragma pack (pop)

class FileSink : public SinkBase
{
public:
	FileSink(const std::string& filenamePrefix);

private:
	void consume(std::unique_ptr<StreamPacket> sp) override;

	std::thread m_outputThread;
	std::fstream m_dataFile;
	std::fstream m_indexFile;
	size_t m_offset = 0;
};

class Recorder
{
public:
	Recorder();
	void readConfig(int argc, char** argv);
	void run();
	void stop();

private:
	void configureDevice();
	void configureSimulator();

	cic::Parameters m_p{
		"Options for Signal Hound Recorder",
		cic::ParametersGroup(
		    "Band",
		    "Band parameters",
		    cic::Parameter<double>("freq-center", "Central recording frequency", SHDevice::centerForMinFreqMaxBW),
		    cic::Parameter<double>("bandwidth", "Recording bandwidth", SHDevice::maxBandwidth)
		),
		cic::ParametersGroup(
		    "Level",
		    "Signal level parameters",
		    cic::Parameter<double>("reference", "Reference level in dBm (\?\?)", SHDevice::defaultReference),
		    cic::Parameter<double>("attenuator", "Attenuation setting in dB. If attenuation provided is negative,"
		                           " attenuation is selected automatically.", SHDevice::defaultAttenuator)
		),
		cic::ParametersGroup(
		    "Streaming",
		    "Files and streams parameters",
		    cic::Parameter<bool>("simulate", "Use simulation instead of real SignalHound device", false, cic::ParamterType::cmdLine),
		    cic::Parameter<std::string>("file-prefix", "Filename prefix", "signal-hound"),
		    cic::Parameter<size_t>("rotation-size", "File rotation size", 50*1024*1024),
		    cic::Parameter<size_t>("max-block-size", "Max size of block readed from device at one time", 10*1024)
		)
	};

	bool m_needRun = true;
	std::unique_ptr<IStreamProvider> m_streamProvider; //DeviceEmulator m_emulator;
	std::unique_ptr<FileSink> m_fileSink;
	std::unique_ptr<Connector> m_connector;
	bool m_stopNow = false;
};

#endif // RECORDER_HPP
