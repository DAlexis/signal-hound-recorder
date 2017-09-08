#include "recorder.hpp"
#include "emulated-device.hpp"
#include "sh-device.hpp"
#include "bb_api.h"

#include <iostream>

using namespace std;

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
                "~/.config/shr/shr.conf",
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

void Recorder::printSummary()
{
    if (m_p["Device simulation"].get<bool>("simulate"))
    {
        cout << "Running DEVICE SIMULATOR with following parameters:" << endl;
    } else {
        cout << "Running SignalHound device with following parameters:" << endl;
    }
    cout << "Band:" << endl
         << "  " << m_p["Band"].get<double>("band-from") << " Hz -- " << m_p["Band"].get<double>("band-to") << " Hz" << endl
         << "Reference:" << endl
         << "  " << m_p["Level"].get<double>("reference") << " dBm" << endl;
    cout << "Attenuator:" << endl;
    if (m_p["Level"].get<double>("attenuator") >= 0)
         cout << "  " << m_p["Level"].get<double>("attenuator") << " dB" << endl;
    else
        cout << "  auto" << endl;
    cout << "Data and stream files prefix:" << endl
         << "  " << m_p["Streaming"].get<std::string>("file-prefix") << endl;
    cout << "==========" << endl;
}

void Recorder::configureDevice()
{
	SHDevice *device = new SHDevice();
	m_streamProvider.reset(device);
    m_connector.reset(new Connector(*m_streamProvider, *m_fileSink));

    device->open();
    double f = m_p["Band"].get<double>("band-from");
    double t = m_p["Band"].get<double>("band-to");
    double center = (f+t)/2.0;
    double span = t-f;
	device->configureCenterSpan(
        center,
        span
	);
	device->configureLevel(
	    m_p["Level"].get<double>("reference"),
	    m_p["Level"].get<double>("attenuator")
	);
	device->configureGain();
	device->runRealTime();
}

void Recorder::configureSimulator()
{
	cout << "Running with device emulation" << endl;
    double f = m_p["Band"].get<double>("band-from");
    double t = m_p["Band"].get<double>("band-to");
    double p = m_p["Device simulation"].get<double>("test-block-period");
    size_t blockSize = 2*(t-f) * p; // Samples per block (during period)
    DeviceEmulator *emulator = new DeviceEmulator(
        p,
        blockSize,
        m_p["Device simulation"].get<double>("sleep-on-receive")
    );

	m_streamProvider.reset(emulator);
	m_connector.reset(new Connector(*m_streamProvider, *m_fileSink));
    cout << "Device simulator configured successfuly" << endl;
}

void Recorder::stop()
{
	m_stopNow = true;
}

void Recorder::run()
{
	if (!m_needRun)
		return;

    printSummary();

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
    cout << "Recorder stopped" << endl;
}

