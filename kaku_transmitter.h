#pragma once

#include <gpiod.hpp>

#include <chrono>
#include <cstdint>
#include <string>

// Klik-aan-klik-uit zender (nieuw "zelflerend" protocol), een port van
// NewRemoteTransmitter uit de Arduino-library NewRemoteSwitch.
//
// Een bericht bestaat uit:
//   startpuls, 26 bits adres, groepsbit, aan/uit-bit, 4 bits unit, stoppuls
class KakuTransmitter {
public:
    // address: 0 t/m 67108863 (26 bits)
    // period:  basispulsduur, standaard 260 µs (zelfde als op je ESP32)
    // repeats: zoals in de library: het bericht wordt 2^repeats keer verzonden
    KakuTransmitter(const std::string& chip_path, unsigned pin, std::uint32_t address,
                    std::chrono::microseconds period = std::chrono::microseconds(260),
                    unsigned repeats = 3);

    void send_unit(unsigned unit, bool on);  // unit 0 t/m 15
    void send_group(bool on);                // alle gekoppelde stopcontacten

private:
    void send(bool group, bool on, unsigned unit);
    void send_bit(bool one);
    void level(bool high, int periods);  // pin zetten en 'periods' x T wachten

    gpiod::line_request request_;
    unsigned pin_;
    std::uint32_t address_;
    std::chrono::nanoseconds period_;
    unsigned transmissions_;
    std::chrono::steady_clock::time_point deadline_{};
};
