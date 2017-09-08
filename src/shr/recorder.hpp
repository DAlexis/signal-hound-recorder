#ifndef RECORDER_HPP
#define RECORDER_HPP

#include "cic.hpp"
#include "communication.hpp"
#include "file-sink.hpp"
#include "sh-device.hpp"
#include "monitor.hpp"

#include <memory>

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
		    cic::Parameter<std::string>("file-prefix", "Filename prefix", "signal-hound"),
		    cic::Parameter<size_t>("rotation-size", "File rotation size", 50*1024*1024),
		    cic::Parameter<size_t>("max-block-size", "Max size of block readed from device at one time", 10*1024)
        ),
        cic::ParametersGroup(
            "Device simulation",
            "Device simulation parameters for testing purposes",
            cic::Parameter<bool>("simulate", "Use simulation instead of real SignalHound device", false, cic::ParamterType::cmdLine),
            cic::Parameter<size_t>("test-block-size", "Size of test block", 1024),
            cic::Parameter<double>("test-block-period", "Period of test block appearing in seconds", 1),
            cic::Parameter<double>("sleep-on-receive", "Sleep after \"data receiving\" from simulated device to make it more slow", 0.1)
        )
	};

	bool m_needRun = true;
	std::unique_ptr<IStreamProvider> m_streamProvider; //DeviceEmulator m_emulator;
    std::unique_ptr<SinkBase> m_fileSink;
	std::unique_ptr<Connector> m_connector;
    std::unique_ptr<Monitor> m_monitor;
	bool m_stopNow = false;
};

#endif // RECORDER_HPP
