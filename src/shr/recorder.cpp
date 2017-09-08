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

void Recorder::configureDevice()
{
	SHDevice *device = new SHDevice();
	m_streamProvider.reset(device);
    m_connector.reset(new Connector(*m_streamProvider, *m_fileSink));

    device->open();
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
}

void Recorder::configureSimulator()
{
	cout << "Running with device emulation" << endl;
    DeviceEmulator *emulator = new DeviceEmulator(
        m_p["Device simulation"].get<double>("test-block-period"),
        m_p["Device simulation"].get<size_t>("test-block-size"),
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

