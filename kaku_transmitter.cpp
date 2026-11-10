#include "kaku_transmitter.h"

#include <sched.h>

#include <stdexcept>

namespace {

// Verhoogt tijdelijk de prioriteit van deze thread, zodat Linux het zenden
// minder vaak onderbreekt. Lukt dat niet (geen rechten), dan gaat alles gewoon
// door met normale prioriteit; de herhalingen vangen eventuele fouten op.
class RealtimePriority {
public:
    RealtimePriority() {
        old_policy_ = sched_getscheduler(0);
        sched_getparam(0, &old_param_);
        sched_param param{};
        param.sched_priority = 50;
        active_ = sched_setscheduler(0, SCHED_FIFO, &param) == 0;
    }
    ~RealtimePriority() {
        if (active_) {
            sched_setscheduler(0, old_policy_, &old_param_);
        }
    }
    RealtimePriority(const RealtimePriority&) = delete;
    RealtimePriority& operator=(const RealtimePriority&) = delete;

private:
    int old_policy_ = SCHED_OTHER;
    sched_param old_param_{};
    bool active_ = false;
};

gpiod::line_request request_output(const std::string& chip_path, unsigned pin) {
    return gpiod::chip(chip_path)
        .prepare_request()
        .set_consumer("kaku-transmitter")
        .add_line_settings(pin, gpiod::line_settings()
            .set_direction(gpiod::line::direction::OUTPUT)
            .set_output_value(gpiod::line::value::INACTIVE))
        .do_request();
}

}  // namespace

KakuTransmitter::KakuTransmitter(const std::string& chip_path, unsigned pin, std::uint32_t address,
                                 std::chrono::microseconds period, unsigned repeats)
    : request_(request_output(chip_path, pin)),
      pin_(pin),
      address_(address),
      period_(period),
      transmissions_(1u << repeats) {
    if (address > 0x3FFFFFF) {
        throw std::invalid_argument("KaKu-adres moet tussen 0 en 67108863 liggen");
    }
}

void KakuTransmitter::send_unit(unsigned unit, bool on) {
    if (unit > 15) {
        throw std::invalid_argument("KaKu-unit moet tussen 0 en 15 liggen");
    }
    send(false, on, unit);
}

void KakuTransmitter::send_group(bool on) {
    send(true, on, 0);
}

void KakuTransmitter::send(bool group, bool on, unsigned unit) {
    RealtimePriority priority;

    for (unsigned i = 0; i < transmissions_; ++i) {
        deadline_ = std::chrono::steady_clock::now();

        // Startpuls: hoog T, laag 10,5 T (de library gebruikt ook 10,5)
        level(true, 1);
        request_.set_value(pin_, gpiod::line::value::INACTIVE);
        deadline_ += period_ * 10 + period_ / 2;
        while (std::chrono::steady_clock::now() < deadline_) {
        }

        for (int bit = 25; bit >= 0; --bit) {
            send_bit((address_ >> bit) & 1u);
        }
        send_bit(group);
        send_bit(on);
        for (int bit = 3; bit >= 0; --bit) {
            send_bit((unit >> bit) & 1u);
        }

        // Stoppuls: hoog T, laag 40 T
        level(true, 1);
        level(false, 40);
    }
}

// Elke bit bestaat uit twee pulsen:
//   '0' = hoog T, laag T,  hoog T, laag 5T
//   '1' = hoog T, laag 5T, hoog T, laag T
void KakuTransmitter::send_bit(bool one) {
    level(true, 1);
    level(false, one ? 5 : 1);
    level(true, 1);
    level(false, one ? 1 : 5);
}

// Zet de pin en wacht actief tot het volgende tijdstip op een vaste tijdlijn.
// Actief wachten is nauwkeuriger dan sleep_for, en door de vaste tijdlijn
// lopen kleine vertragingen niet op over het hele bericht.
void KakuTransmitter::level(bool high, int periods) {
    request_.set_value(pin_, high ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
    deadline_ += period_ * periods;
    while (std::chrono::steady_clock::now() < deadline_) {
    }
}
