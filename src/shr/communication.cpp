#include "communication.hpp"

#include <iostream>

using namespace std;

///////////////////////////////////////////
/// StreamPacket

StreamPacket::StreamPacket(size_t size)
{
    data.resize(size*2, 0.0);

    packet.iqData = data.data();
    packet.iqCount = size;
    packet.triggers = nullptr;
    packet.triggerCount = 0;
    packet.purge = BB_FALSE;
    packet.dataRemaining = 0;
    packet.sampleLoss = 0;
    packet.sec = 0;
    packet.nano = 0;
}

///////////////////////////////////////////
/// SinkBase

void SinkBase::enqueue(std::unique_ptr<StreamPacket> sp)
{
    //cout << "SinkBase::enqueue" << endl;
    std::unique_lock<std::mutex> lck(m_queueMutex);
    m_queue.push(std::move(sp));
    m_queueCv.notify_all();
}

void SinkBase::run()
{
    m_consumerThread = std::thread([this](){ consumerLoop(); });
}

void SinkBase::stop()
{
    m_needStop = true;
    m_queueCv.notify_all();

    if (m_consumerThread.joinable())
        m_consumerThread.join();
}

void SinkBase::consumerLoop()
{
    cout << "Running consumer loop" << endl;
    std::queue<std::unique_ptr<StreamPacket>> toWrite;
    while (!m_needStop)
    {
        std::unique_lock<std::mutex> lck(m_queueMutex);
        if (m_queue.empty())
            m_queueCv.wait(lck, [this](){ return m_needStop || !m_queue.empty(); });

        // Moving all content from queue and quickly unlocking it
        toWrite.swap(m_queue);
        lck.unlock();

        //std::this_thread::sleep_for(std::chrono::seconds(3));
        while (!toWrite.empty())
        {
            std::unique_ptr<StreamPacket> psp = std::move(toWrite.front());
            toWrite.pop();
            consume(std::move(psp));
        }
    }
}


///////////////////////////////////////////
/// Connector

Connector::Connector(IStreamProvider& provider, SinkBase& sink) :
    m_provider(provider),
    m_sink(sink)
{

}

void Connector::tick(size_t blockSize)
{
    std::unique_ptr<StreamPacket> p = m_provider.getPacket(blockSize);
    if (p.get() != nullptr && p->packet.iqCount != 0)
    {
        m_sink.enqueue(std::move(p));
    }
}

