/*
 * libdatachannel streamer example
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NalUnit_hpp
#define NalUnit_hpp

#include "include.hpp"

#if RTC_ENABLE_MEDIA

namespace rtc {

#pragma pack(push, 1)

/// Nalu header
struct NalUnitHeader {
    bool forbiddenBit() { return _first >> 7; }
    uint8_t nri() { return _first >> 5 & 0x03; }
    uint8_t unitType() { return _first & 0x1F; }

    void setForbiddenBit(bool isSet) { _first = (_first & 0x7F) | (isSet << 7); }
    void setNRI(uint8_t nri) { _first = (_first & 0x9F) | ((nri & 0x03) << 5); }
    void setUnitType(uint8_t type) { _first = (_first &0xE0) | (type & 0x1F); }
private:
    uint8_t _first = 0;
};

/// Nalu fragment header
struct NalUnitFragmentHeader {
    bool isStart() { return _first >> 7; }
    bool reservedBit6() { return (_first >> 6) & 0x01; }
    bool isEnd() { return (_first >> 5) & 0x01; }
    uint8_t unitType() { return _first & 0x1F; }

    void setStart(bool isSet) { _first = (_first & 0x7F) | (isSet << 7); }
    void setEnd(bool isSet) { _first = (_first & 0xDF) | (isSet << 6); }
    void setReservedBit6(bool isSet) { _first = (_first & 0xBF) | (isSet << 5); }
    void setUnitType(uint8_t type) { _first = (_first &0xE0) | (type & 0x1F); }
private:
    uint8_t _first = 0;
};

#pragma pack(pop)

/// Nal unit
struct NalUnit: rtc::binary {
    NalUnit(const NalUnit &unit) = default;
    NalUnit(size_t size, bool includingHeader = true): rtc::binary(size + (includingHeader ? 0 : 1)) { }

    template <typename Iterator>
    NalUnit(Iterator begin_, Iterator end_): rtc::binary(begin_, end_) { }

    NalUnit(rtc::binary &&data) : rtc::binary(std::move(data)) { }

    bool forbiddenBit() { return header()->forbiddenBit(); }
    uint8_t nri() { return header()->nri(); }
    uint8_t unitType() { return header()->unitType(); }
    rtc::binary payload() {
        assert(size() >= 1);
        return {begin() + 1, end()};
    }

    void setForbiddenBit(bool isSet) { header()->setForbiddenBit(isSet); }
    void setNRI(uint8_t nri) { header()->setNRI(nri); }
    void setUnitType(uint8_t type) { header()->setUnitType(type); }
    void setPayload(rtc::binary payload) {
        assert(size() >= 1);
        erase(begin() + 1, end());
        insert(end(), payload.begin(), payload.end());
    }

protected:
    NalUnitHeader * header() {
        assert(size() >= 1);
        return (NalUnitHeader *) data();
    }
};

/// Nal unit fragment A
struct NalUnitFragmentA: NalUnit {
    enum class FragmentType {
        Start,
        Middle,
        End
    };

    NalUnitFragmentA(FragmentType type, bool forbiddenBit, uint8_t nri, uint8_t unitType, rtc::binary data);

    static std::vector<NalUnitFragmentA> fragmentsFrom(NalUnit nalu, uint16_t maximumFragmentSize);

    uint8_t unitType() { return fragmentHeader()->unitType(); }

    rtc::binary payload() {
        assert(size() >= 2);
        return {begin() + 2, end()};
    }

    FragmentType type() {
        if(fragmentHeader()->isStart()) {
            return FragmentType::Start;
        } else if(fragmentHeader()->isEnd()) {
            return FragmentType::End;
        } else {
            return FragmentType::Middle;
        }
    }

    void setUnitType(uint8_t type) { fragmentHeader()->setUnitType(type); }

    void setPayload(rtc::binary payload) {
        assert(size() >= 2);
        erase(begin() + 2, end());
        insert(end(), payload.begin(), payload.end());
    }

    void setFragmentType(FragmentType type);

protected:
    NalUnitHeader * fragmentIndicator() {
        return (NalUnitHeader *) data();
    }

    NalUnitFragmentHeader * fragmentHeader() {
        return (NalUnitFragmentHeader *) fragmentIndicator() + 1;
    }

    const uint8_t nal_type_fu_A = 28;
};

class NalUnits: public std::vector<NalUnit> {
public:
    static const uint16_t defaultMaximumFragmentSize = 1100;
    std::vector<rtc::binary> generateFragments(uint16_t maximumFragmentSize = NalUnits::defaultMaximumFragmentSize);
};

} // namespace

#endif /* RTC_ENABLE_MEDIA */

#endif /* NalUnit_hpp */
