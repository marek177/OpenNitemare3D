#include "parity/BinaryParity.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
int main() {
    using namespace n3d::parity;
    std::vector<std::uint8_t> a{0,1,2,0xff};
    LosslessSnapshot s(a);
    assert(s.serialize()==a && s.at(3)==0xff);
    s.patch(1,42); assert(s.at(1)==42);
    assert(s.serialize()[0]==0 && s.serialize()[3]==0xff);
    assert(compare(a,a).equal);
    auto d=compare(a,s.bytes());
    assert(!d.equal && d.first_byte_difference && d.first_byte_difference->offset==1);
    auto short_a=std::vector<std::uint8_t>{0,1};
    assert(!compare(a,short_a).equal);
    assert(!compare(std::vector<std::uint8_t>{},a).equal);
    std::vector<std::uint8_t> frame(64000), dac(768);
    assert(exact_indexed_frame_and_dac(frame,dac,frame,dac));
    frame[63999]=1;
    assert(!exact_indexed_frame_and_dac(std::vector<std::uint8_t>(64000),dac,frame,dac));
    frame[63999]=0; dac[767]=1;
    assert(!exact_indexed_frame_and_dac(frame,std::vector<std::uint8_t>(768),frame,dac));
    assert(!exact_indexed_frame_and_dac(std::vector<std::uint8_t>(63999),dac,frame,dac));
}
