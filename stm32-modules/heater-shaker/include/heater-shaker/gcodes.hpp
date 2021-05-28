/*
** Definitions of valid gcodes understood by the heater/shaker; intended to work
*with
** the gcode parser in gcode_parser.hpp
*/

#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <optional>
#include <utility>

#include "heater-shaker/gcode_parser.hpp"
#include "heater-shaker/utility.hpp"

namespace gcode {

struct SetRPM {
    /*
    ** Set RPM uses the spindle-speed code from standard gcode, M3 (CW)
    ** Format: M3 S<RPM>
    ** Example: M3 S500 sets target rpm to 500
    */
    using ParseResult = std::optional<SetRPM>;
    static constexpr auto prefix = std::array{'M', '3', ' ', 'S'};
    static constexpr const char* response = "M3 OK\n";
    int16_t rpm;

    template <typename InputIt, typename InputLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InputLimit, InputIt> static auto
        write_response_into(InputIt buf, InputLimit limit) -> InputIt {
        return write_string_to_iterpair(buf, limit, response);
    }

    template <typename InputIt, typename Limit>
    requires std::contiguous_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        // minimal m3 command

        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }

        auto value_res = parse_value<int16_t>(working, limit);

        if (!value_res.first.has_value()) {
            return std::make_pair(ParseResult(), input);
        }
        return std::make_pair(ParseResult(SetRPM{.rpm = static_cast<int16_t>(
                                                     value_res.first.value())}),
                              value_res.second);
    }
};

struct SetTemperature {
    /*
    ** SetTemperature uses a standard set-tool-temperature gcode, M104
    ** Format: M104 S<temp>
    ** Example: M104 S25 sets target temperature to 25C
    */
    using ParseResult = std::optional<SetTemperature>;
    static constexpr auto prefix = std::array{'M', '1', '0', '4', ' ', 'S'};
    static constexpr const char* response = "M104 OK\n";
    double temperature;

    template <typename InputIt, typename InLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InLimit, InputIt> static auto
        write_response_into(InputIt buf, InLimit limit) -> InputIt {
        return write_string_to_iterpair(buf, limit, response);
    }

    template <typename InputIt, typename Limit>
    requires std::contiguous_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }

        auto value_res = parse_value<float>(working, limit);

        if (!value_res.first.has_value()) {
            return std::make_pair(ParseResult(), input);
        }

        if (value_res.first.value() < 0) {
            return std::make_pair(ParseResult(), input);
        }

        return std::make_pair(
            ParseResult(SetTemperature{.temperature = value_res.first.value()}),
            value_res.second);
    }
};

struct GetTemperature {
    /*
    ** GetTemperature keys off a standard get-tool-temperature gcode, M105
    ** Format: M105
    ** Example: M105
    */
    using ParseResult = std::optional<GetTemperature>;
    static constexpr auto prefix = std::array{'M', '1', '0', '5'};

    template <typename InputIt, typename InLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InputIt, InLimit> static auto
        write_response_into(InputIt buf, InLimit limit,
                            double current_temperature,
                            double setpoint_temperature) -> InputIt {
        auto res = snprintf(&*buf, (limit - buf), "M105 C%0.2f T%0.2f OK\n",
                            static_cast<float>(current_temperature),
                            static_cast<float>(setpoint_temperature));
        if (res <= 0) {
            return buf;
        }
        return buf + res;
    }
    template <typename InputIt, typename Limit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }
        if (working != limit && !std::isspace(*working)) {
            return std::make_pair(ParseResult(), input);
        }
        return std::make_pair(ParseResult(GetTemperature()), working);
    }
};

struct GetRPM {
    /*
    ** GetRPM keys off a random gcode that sometimes does the right thing since
    *it's not
    ** like it's standardized or anything, M123
    ** Format: M123
    ** Example: M123
    */
    using ParseResult = std::optional<GetRPM>;
    static constexpr auto prefix = std::array{'M', '1', '2', '3'};

    template <typename InputIt, typename InLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InLimit, InputIt> static auto
        write_response_into(InputIt buf, const InLimit limit,
                            int16_t current_rpm, int16_t setpoint_rpm)
            -> InputIt {
        static constexpr const char* prefix = "M123 C";
        char* char_next = &*buf;
        char* const char_limit = &*limit;
        char_next = write_string_to_iterpair(char_next, char_limit, prefix);

        auto tochars_result = std::to_chars(char_next, char_limit, current_rpm);
        if (tochars_result.ec != std::errc()) {
            return buf + (tochars_result.ptr - &*buf);
        }
        char_next = tochars_result.ptr;

        static constexpr const char* setpoint_prefix = " T";
        char_next =
            write_string_to_iterpair(char_next, char_limit, setpoint_prefix);

        tochars_result = std::to_chars(char_next, char_limit, setpoint_rpm);
        if (tochars_result.ec != std::errc()) {
            return buf + (tochars_result.ptr - &*buf);
        }
        char_next = tochars_result.ptr;

        static constexpr const char* suffix = " OK\n";
        char_next = write_string_to_iterpair(char_next, char_limit, suffix);
        return buf + (char_next - &*buf);
    }

    template <typename InputIt, typename Limit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }
        return std::make_pair(ParseResult(GetRPM()), working);
    }
};

struct SetAcceleration {
    /*
    ** SetAcceleration uses M204 which is kind of the right thing. The
    *acceleration is in
    ** RPM/s.
    **
    ** Note: The spindle doesn't use linear acceleration all the time. This is
    *the ramp rate
    ** that will be followed for the majority of the time spent changing speeds.
    *It may be
    ** different when blending between ramp and constant speed control.
    ** Format: M204 Sxxxx
    ** Example: M204 S10000
    */

    using ParseResult = std::optional<SetAcceleration>;
    int32_t rpm_per_s;
    static constexpr auto prefix = std::array{'M', '2', '0', '4', ' ', 'S'};
    static constexpr const char* response = "M204 OK\n";

    template <typename InputIt, typename InputLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InputLimit, InputIt> static auto
        write_response_into(InputIt buf, InputLimit limit) -> InputIt {
        return write_string_to_iterpair(buf, limit, response);
    }

    template <typename InputIt, typename Limit>
    requires std::contiguous_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        // minimal m3 command

        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }

        auto value_res = parse_value<int32_t>(working, limit);

        if (!value_res.first.has_value()) {
            return std::make_pair(ParseResult(), input);
        }
        return std::make_pair(
            ParseResult(SetAcceleration{
                .rpm_per_s = static_cast<int32_t>(value_res.first.value())}),
            value_res.second);
    }
};

struct GetTemperatureDebug {
    /**
     * GetTemperatureDebug uses M105.D arbitrarily. It responds with
     *
     * - Pad A temperature (AT)
     * - Pad B temperature (BT)
     * - Board temperature (OT)
     * - Pad A last ADC reading (AD)
     * - Pad B last ADC reading (BD)
     * - Board last ADC reading (OD)
     * - power good (PG)
     * */
    using ParseResult = std::optional<GetTemperatureDebug>;
    static constexpr auto prefix = std::array{'M', '1', '0', '5', '.', 'D'};

    template <typename InputIt, typename InLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InputIt, InLimit> static auto
        write_response_into(InputIt buf, InLimit limit, double pad_a_temp,
                            double pad_b_temp, double board_temp,
                            uint16_t pad_a_adc, uint16_t pad_b_adc,
                            uint16_t board_adc, bool power_good) -> InputIt {
        auto res = snprintf(
            &*buf, (limit - buf),
            "M105.D AT%0.2f BT%0.2f OT%0.2f AD%d BD%d OD%d PG%d OK\n",
            static_cast<float>(pad_a_temp), static_cast<float>(pad_b_temp),
            static_cast<float>(board_temp), pad_a_adc, pad_b_adc, board_adc,
            power_good ? 1 : 0);
        if (res <= 0) {
            return buf;
        }
        return buf + res;
    }
    template <typename InputIt, typename Limit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }
        return std::make_pair(ParseResult(GetTemperatureDebug()), working);
    }
};

struct SetHeaterPIDConstants {
    /**
     * SetHeaterPIDConstants uses M301 because smoothieware does. Parameters:
     * Pxxx.xxx Ixxx.xxx Dxxx.xxx
     *
     * Example: M301 P1.02 I2.1 D1.0\r\n
     * */
    double kp;
    double ki;
    double kd;

    using ParseResult = std::optional<SetHeaterPIDConstants>;
    static constexpr auto prefix = std::array{'M', '3', '0', '1', ' ', 'P'};
    static constexpr const char* response = "M301 OK\n";

    template <typename InputIt, typename InLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InputIt, InLimit> static auto
        write_response_into(InputIt buf, InLimit limit) -> InputIt {
        return write_string_to_iterpair(buf, limit, response);
    }
    template <typename InputIt, typename Limit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }

        auto kp_res = parse_value<float>(working, limit);

        if (!kp_res.first.has_value()) {
            return std::make_pair(ParseResult(), input);
        }

        constexpr auto i_pref = std::array{' ', 'I'};
        working = prefix_matches(kp_res.second, limit, i_pref);
        if (working == kp_res.second) {
            return std::make_pair(ParseResult(), input);
        }

        auto ki_res = parse_value<float>(working, limit);

        if (!ki_res.first.has_value()) {
            return std::make_pair(ParseResult(), input);
        }

        constexpr auto kd_pref = std::array{' ', 'D'};
        working = prefix_matches(ki_res.second, limit, kd_pref);
        if (working == ki_res.second) {
            return std::make_pair(ParseResult(), input);
        }
        auto kd_res = parse_value<float>(working, limit);
        if (!kd_res.first.has_value()) {
            return std::make_pair(ParseResult(), input);
        }

        return std::make_pair(
            ParseResult(SetHeaterPIDConstants{.kp = kp_res.first.value(),
                                              .ki = ki_res.first.value(),
                                              .kd = kd_res.first.value()}),
            kd_res.second);
    }
};

struct SetHeaterPowerTest {
    /**
     * SetHeaterPowerTest is a testing command to directly command heater power
     * It uses M104.D to be like SetTemperature since it's the same kind of
     *thing.
     *
     * The argument should be between 1 and 0.
     * The power will be maintained at the specified level until either
     * - An error occurs
     * - An M104 is sent
     * - Another M104.D is sent
     *
     * A command of exactly 0 will turn off the power.
     *
     * While the system is in power test mode, M105 will return the power
     *setting as its target temperature, rather than a target temperature value.
     *The current temperature will still be the current temperature in C.
     *
     * Command: M104.D S0.124\n
     **/
    double power;
    using ParseResult = std::optional<SetHeaterPowerTest>;
    static constexpr auto prefix =
        std::array{'M', '1', '0', '4', '.', 'D', ' ', 'S'};
    static constexpr const char* response = "M104.D OK\n";

    template <typename InputIt, typename InLimit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<InputIt, InLimit> static auto
        write_response_into(InputIt buf, InLimit limit) -> InputIt {
        return write_string_to_iterpair(buf, limit, response);
    }
    template <typename InputIt, typename Limit>
    requires std::forward_iterator<InputIt>&&
        std::sized_sentinel_for<Limit, InputIt> static auto
        parse(const InputIt& input, Limit limit)
            -> std::pair<ParseResult, InputIt> {
        auto working = prefix_matches(input, limit, prefix);
        if (working == input) {
            return std::make_pair(ParseResult(), input);
        }

        auto power = parse_value<float>(working, limit);
        if (!power.first.has_value()) {
            return std::make_pair(ParseResult(), input);
        }
        auto power_val = power.first.value();
        if ((power_val < 0) || (power_val > 1.0)) {
            return std::make_pair(ParseResult(), input);
        }
        return std::make_pair(
            ParseResult(SetHeaterPowerTest{.power = power_val}), power.second);
    }
};

}  // namespace gcode
