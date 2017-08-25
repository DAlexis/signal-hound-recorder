#include "recorder.hpp"

#include "cic.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <signal.h>

using namespace std;
using namespace cic;

Recorder r;

void signalCallbackHandler(int signum)
{
	switch(signum)
	{
	    case SIGINT:
		    cerr << "Interrupt signal" << endl;
		break;
	    case SIGTERM:
		    cerr << "Terminate signal" << endl;
		break;
	}
	r.stop();
}

int main(int argc, char** argv)
{
	signal(SIGINT, signalCallbackHandler);
	signal(SIGTERM, signalCallbackHandler);

	r.readConfig(argc, argv);
	r.run();
	/*try {
		r.readConfig(argc, argv);
		r.run();
	} catch (std::exception& ex)
	{
		cerr << "Exception during execution: " << ex.what() << endl;
		return -1;
	}*/
	return 0;

/*
    bbStatus status;
	int handle = -1;

	status = bbOpenDevice(&handle);
	if(status != bbNoError) {
		// Unable to open device
		std::cout << "Unable to open device\n";
		std::cout << bbGetErrorString(status) << std::endl;
		return 0;
	} else {
		std::cout << "Device opened successfully\n";
	}

	bbConfigureCenterSpan(handle, 3.01e9, 5.98e9);
	bbConfigureLevel(handle, -20.0, BB_AUTO_ATTEN);
	bbConfigureGain(handle, BB_AUTO_GAIN);
	//bbConfigureIO
	//bbConfigureIQ
	bbInitiate(handle, BB_STREAMING, BB_STREAM_IQ);
*/
/*
	std::vector<float> min, max;
	unsigned int len;
	double start, bin;

	bbConfigureAcquisition(handle, BB_MIN_AND_MAX, BB_LOG_SCALE);
	bbConfigureCenterSpan(handle, 3.01e9, 5.98e9);
	bbConfigureLevel(handle, -20.0, BB_AUTO_ATTEN);
	bbConfigureGain(handle, BB_AUTO_GAIN);
	bbConfigureSweepCoupling(handle, 100.0e3, 100.0e3, 0.001,
			   BB_NON_NATIVE_RBW, BB_NO_SPUR_REJECT);

	bbInitiate(handle, BB_SWEEPING, 0);
	bbQueryTraceInfo(handle, &len, &bin, &start);

	min.resize(len);
	max.resize(len);

	bbFetchTrace_32f(handle, len, &min[0], &max[0]);

	std::vector<float>::iterator iter =
	std::max_element(max.begin(), max.end());

	std::cout << "Peak Freq = "
		<< (start + bin * (iter - max.begin())) * 1.0e-6
		<< " MHz"
		<< std::endl;
	std::cout << "Peak Amp = "
		<< *iter << " dBm"
		<< std::endl;
*/
/*
	bbAbort(handle);
	bbCloseDevice(handle);

*/
    return 0;
}
