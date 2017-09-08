#ifndef FILESINK_HPP
#define FILESINK_HPP

#include "communication.hpp"

#include <fstream>

#pragma pack (push, 0)
struct HeaderSerialized {
    HeaderSerialized(StreamPacket& pkt, size_t offset, size_t triggersOffset = 0);

    size_t offset;
    int iqCount;
    size_t triggersOffset;
    int triggerCount;
    int purge;
    int dataRemaining;
    int sampleLoss;
    int sec;
    int nano;
};
#pragma pack (pop)

class FileSink : public SinkBase
{
public:
    FileSink(const std::string& filenamePrefix);
    size_t currentSize() const override;

private:
    void consume(std::unique_ptr<StreamPacket> sp) override;

    std::thread m_outputThread;
    std::fstream m_dataFile;
    std::fstream m_indexFile;
    size_t m_offset = 0;
};

#endif // FILESINK_HPP
