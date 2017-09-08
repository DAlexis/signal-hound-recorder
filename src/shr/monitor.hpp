#ifndef MONITOR_HPP
#define MONITOR_HPP

#include "communication.hpp"

#include <thread>

class Monitor
{
public:
    Monitor(const IStreamProvider* provider, const SinkBase* sink, const Connector* connector);

    void run();
    void stop();

private:
    void mainLoop();
    void printStatus();
    bool waitOrStop(int milliseconds);

    std::thread m_thread;

    const IStreamProvider* m_provide;
    const SinkBase* m_sink;
    const Connector* m_connector;
    bool m_needStop = false;
};


#endif // MONITOR_HPP
