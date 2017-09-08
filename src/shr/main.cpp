#include "recorder.hpp"

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
	return 0;
}
