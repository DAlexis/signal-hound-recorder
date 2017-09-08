#include "file-sink.hpp"

using namespace std;

///////////////////////////////////////////
/// HeaderSerialized

HeaderSerialized::HeaderSerialized(StreamPacket& pkt, size_t offset, size_t triggersOffset)
{
    this->offset = offset;
    iqCount = pkt.packet.iqCount;
    triggersOffset = triggersOffset;
    triggerCount = pkt.packet.triggerCount;
    purge = pkt.packet.purge;
    dataRemaining = pkt.packet.dataRemaining;
    sampleLoss = pkt.packet.sampleLoss;
    sec = pkt.packet.sec;
    nano = pkt.packet.nano;
}

///////////////////////////////////////////
/// FileSink

FileSink::FileSink(const std::string& filename)
    : m_dataFile(filename + "_data.bin", ios::out),
      m_indexFile(filename + "_index.bin", ios::out)
{
}

size_t FileSink::currentSize() const
{
    return m_offset;
}

void FileSink::consume(std::unique_ptr<StreamPacket> sp)
{
    //cout << "FileSink::consume" << endl;
    HeaderSerialized hs(*sp, m_offset);
    m_indexFile.write(reinterpret_cast<const char*>(&hs), sizeof(hs));
    m_dataFile.write(reinterpret_cast<const char*>(sp->data.data()), sizeof(sp->data[0]) * 2 * sp->packet.iqCount);
    //cout << "index: " << sizeof(hs) << " data: " << sizeof(sp->data[0]) * 2 * sp->packet.iqCount << endl;
    m_offset += sp->packet.iqCount;
}
