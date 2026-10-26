#include "formats/DemoFile.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {
void u16(std::vector<std::uint8_t>& out, std::uint16_t n) {
    out.push_back(static_cast<std::uint8_t>(n));
    out.push_back(static_cast<std::uint8_t>(n >> 8));
}
void u32(std::vector<std::uint8_t>& out, std::uint32_t n) {
    u16(out, static_cast<std::uint16_t>(n));
    u16(out, static_cast<std::uint16_t>(n >> 16));
}
void rec(std::vector<std::uint8_t>& out, std::uint16_t k, std::uint16_t f, std::uint32_t t) {
    u16(out,k); u16(out,f); u32(out,t);
}
}

int main() {
    // Synthetic fixture: no original commercial bytes are checked in.
    // Both key bytes matter: old 1+2+1+4 decoder falsely read 0xE0
    // as a single-byte event and shifted the flags by one byte.
    std::vector<std::uint8_t> bytes;
    u16(bytes,15); u16(bytes,7); u16(bytes,30);
    rec(bytes,0x48E0,0x0002,8);
    rec(bytes,0x74E0,0x0090,1074);
    auto path = std::filesystem::temp_directory_path() / "opennitemare3d_demo_eof_regression_fixture.bin";
    { std::ofstream output(path,std::ios::binary);
      output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())); }
    auto demo = n3d::DemoFile::load(path);
    std::filesystem::remove(path);
    assert(demo.rawHeaderWords()==(std::array<std::uint16_t,3>{15,7,30}));
    assert(demo.records().size()==2);
    assert(demo.records()[0].key==0x48E0 && demo.records()[0].flags==0x0002);
    assert(demo.records()[1].key==0x74E0 && demo.records()[1].flags==0x0090);
    assert(demo.renderGenerationsAreMonotonic());

    n3d::DemoReaderState s;
    std::uint16_t read=0xFFFF;
    assert(!s.applyDue(demo,7,read) && read==0xFFFF && s.nextRecordIndex==0);
    assert(s.applyDue(demo,8,read) && read==8 && s.nextRecordIndex==1);
    assert(s.lastKey==0x48E0 && s.lastFlags==2);
    assert(s.lastPrefetched==&demo.records()[1]);
    assert(!s.firstEofObserved);
    assert(!s.applyDue(demo,1073,read));
    assert(s.applyDue(demo,1074,read) && read==0);
    assert(s.firstEofObserved && s.nextRecordIndex==2);
    assert(s.lastKey==0x74E0 && s.lastFlags==0x0090);
    assert(s.lastPrefetched==&demo.records()[1]);
    assert(!s.applyDue(demo,1075,read));
    assert(read==0 && s.lastKey==0x74E0 && s.lastFlags==0x0090);

    // Synthetic 275-record application order/read-length invariant.
    // It is NOT a replay of the unavailable original 734-event ledger.
    std::vector<std::uint8_t> longBytes(bytes.begin(),bytes.begin()+6);
    for (std::uint32_t i=0;i<275;++i) rec(longBytes,static_cast<std::uint16_t>(i),0,i+2);
    { std::ofstream output(path,std::ios::binary);
      output.write(reinterpret_cast<const char*>(longBytes.data()),static_cast<std::streamsize>(longBytes.size())); }
    auto many=n3d::DemoFile::load(path);
    std::filesystem::remove(path);
    n3d::DemoReaderState q;
    for (std::size_t i=0;i<275;++i) {
        assert(q.applyDue(many,static_cast<std::uint32_t>(i+2),read));
        assert(q.lastKey==i && q.nextRecordIndex==i+1);
        assert(read==(i==274?0:8));
    }
    assert(q.firstEofObserved && q.lastPrefetched==&many.records()[274]);
}
