#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include "bb_api.h"
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

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
    /**
     * @brief monitorGetNotReaded Function is used for monitoring only to estimate count of not readed samples
     * @return Count of not readed samples or zero
     */
    virtual size_t monitorGetNotReaded() const = 0;
};

class SinkBase
{
public:
    virtual ~SinkBase() {}
    void enqueue(std::unique_ptr<StreamPacket> sp);

    void run();
    void stop();

    virtual size_t currentSize() const { return 0; }

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

#endif // COMMUNICATION_HPP
