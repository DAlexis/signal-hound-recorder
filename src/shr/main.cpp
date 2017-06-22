#include <boost/program_options.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

#include "bb_api.h"

using namespace std;

int main(int argc, char** argv)
{
    namespace po = boost::program_options;
    po::options_description generalOptions("Genral options");
    generalOptions.add_options()
        ("help,h", "Print help message");
    
    po::options_description allOptions("Allowed options");
    allOptions
        .add(generalOptions);
    
    po::variables_map vmOptions;
    try
    {
        po::store(po::parse_command_line(argc, argv, allOptions), vmOptions);
        po::notify(vmOptions);
    }
    catch (po::error& e)
    {
        cerr << "Command line parsing error: " << e.what() << endl;
        return -1;
    }
    
    if (vmOptions.count("help"))
    {
        cout << allOptions << endl;
        return 0;
    }
    

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
	bbAbort(handle);
	bbCloseDevice(handle);


    return 0;
}
