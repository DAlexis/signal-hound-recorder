#include "monitor.hpp"

#include <iostream>

using namespace std;

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
