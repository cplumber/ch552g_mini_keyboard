#include "../common/macropad_hid.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr uint8_t kReportId = 6;
constexpr uint8_t kGetButton = 1;
constexpr uint8_t kBeginUpdate = 2;
constexpr uint8_t kSetButton = 3;
constexpr uint8_t kCommit = 4;
constexpr uint8_t kAbort = 5;
constexpr uint8_t kResetDefaults = 6;
constexpr uint8_t kSetDelay = 8;
constexpr uint8_t kGetBoardId = 9;
constexpr uint8_t kOk = 0;
constexpr uint8_t kCtrl = 0x01;
constexpr uint8_t kShift = 0x02;
constexpr uint8_t kAlt = 0x04;
constexpr uint8_t kGui = 0x08;

constexpr const char *kProfiles[] = {"copy_paste", "google_meet", "vs_code", "ms_teams", "test"};
constexpr const char *kButtons[] = {"BTN_1", "BTN_2", "BTN_3", "BTN_4", "BTN_5", "BTN_6"};

struct Macro
{
    uint8_t length = 0;
    uint8_t modifiers[2] = {};
    uint8_t keys[2] = {};
    uint8_t inter_chord_delay_ms = 0;
};

bool exchange(MacropadHid &hid, uint8_t command, uint8_t profile, uint8_t button,
              const Macro *macro, std::array<uint8_t, 9> *reply)
{
    std::array<uint8_t, 9> request = {};
    std::array<uint8_t, 9> response = {};
    request[0] = kReportId;
    request[1] = command;
    request[2] = profile;
    request[3] = button;
    if (macro)
    {
        request[4] = macro->length;
        if (command == kSetDelay)
        {
            request[4] = macro->inter_chord_delay_ms;
        }
        else
        {
            request[4] = macro->length;
            request[5] = macro->modifiers[0];
            request[6] = macro->keys[0];
            request[7] = macro->modifiers[1];
            request[8] = macro->keys[1];
        }
    }

    std::string error;
    if (!hid.exchange_config(request, response, &error))
    {
        std::cerr << "Device communication failed: " << error << "\n";
        return false;
    }
    if (response[1] != kOk)
    {
        std::cerr << "Device rejected command " << static_cast<unsigned>(command)
                  << " with status " << static_cast<unsigned>(response[1]) << "\n";
        return false;
    }
    if (reply)
    {
        *reply = response;
    }
    return true;
}

std::string read_file(const std::string &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return {};
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

bool write_file(const std::string &path, const std::string &contents)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return false;
    }
    output << contents;
    return static_cast<bool>(output);
}

size_t matching(const std::string &text, size_t start, char open, char close)
{
    int depth = 0;
    bool quoted = false;
    for (size_t i = start; i < text.size(); ++i)
    {
        if (text[i] == '"' && (i == 0 || text[i - 1] != '\\'))
        {
            quoted = !quoted;
        }
        if (quoted)
        {
            continue;
        }
        if (text[i] == open)
        {
            ++depth;
        }
        else if (text[i] == close && --depth == 0)
        {
            return i;
        }
    }
    return std::string::npos;
}

bool parse_chord(const std::string &text, size_t &pos, uint8_t &modifiers, uint8_t &key)
{
    modifiers = 0;
    key = 0;
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
    if (pos >= text.size() || text[pos++] != '[') return false;
    while (true)
    {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
        if (pos >= text.size() || text[pos++] != '"') return false;
        const size_t end = text.find('"', pos);
        if (end == std::string::npos) return false;
        const std::string token = text.substr(pos, end - pos);
        pos = end + 1;
        if (token == "Ctrl") modifiers |= kCtrl;
        else if (token == "Shift") modifiers |= kShift;
        else if (token == "Alt") modifiers |= kAlt;
        else if (token == "GUI") modifiers |= kGui;
        else if (token.size() == 1 && static_cast<unsigned char>(token[0]) < 128)
            key = static_cast<uint8_t>(std::tolower(static_cast<unsigned char>(token[0])));
        else return false;

        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
        if (pos >= text.size()) return false;
        if (text[pos] == ']')
        {
            ++pos;
            return key != 0;
        }
        if (text[pos++] != ',') return false;
    }
}

bool parse_macro(const std::string &section, const char *button_name, Macro &macro)
{
    const std::string marker = std::string("\"") + button_name + "\"";
    const size_t key_position = section.find(marker);
    if (key_position == std::string::npos) return false;
    const size_t object = section.find('{', key_position + marker.size());
    if (object == std::string::npos) return false;
    const size_t object_end = matching(section, object, '{', '}');
    if (object_end == std::string::npos) return false;
    const std::string value = section.substr(object, object_end - object + 1);
    const size_t chords_key = value.find("\"chords\"");
    if (chords_key == std::string::npos) return false;
    const size_t outer = value.find('[', chords_key + 8);
    if (outer == std::string::npos) return false;
    const size_t outer_end = matching(value, outer, '[', ']');
    if (outer_end == std::string::npos) return false;

    macro = {};
    size_t pos = outer + 1;
    while (pos < outer_end)
    {
        while (pos < outer_end && (std::isspace(static_cast<unsigned char>(value[pos])) || value[pos] == ',')) ++pos;
        if (pos >= outer_end) break;
        if (macro.length >= 2 || !parse_chord(value, pos, macro.modifiers[macro.length], macro.keys[macro.length]))
            return false;
        ++macro.length;
    }
    const size_t delay_key = value.find("\"between_chords_ms\"");
    if (delay_key == std::string::npos) return false;
    size_t delay_value = value.find(':', delay_key + 19);
    if (delay_value == std::string::npos) return false;
    ++delay_value;
    while (delay_value < value.size() && std::isspace(static_cast<unsigned char>(value[delay_value]))) ++delay_value;
    unsigned int delay = 0;
    size_t digits = 0;
    while (delay_value < value.size() && std::isdigit(static_cast<unsigned char>(value[delay_value])))
    {
        delay = delay * 10 + static_cast<unsigned int>(value[delay_value++] - '0');
        if (++digits > 3 || delay > 255) return false;
    }
    if (digits == 0) return false;
    macro.inter_chord_delay_ms = static_cast<uint8_t>(delay);
    return macro.length <= 2 && (macro.length == 2 || macro.inter_chord_delay_ms == 0);
}

bool parse_config(const std::string &json, std::array<std::array<Macro, 6>, 5> &config)
{
    const size_t version_key = json.find("\"version\"");
    size_t version_value = version_key == std::string::npos
                               ? std::string::npos
                               : json.find(':', version_key + 9);
    if (version_value == std::string::npos)
    {
        return false;
    }
    ++version_value;
    while (version_value < json.size() && std::isspace(static_cast<unsigned char>(json[version_value])))
        ++version_value;
    if (version_value >= json.size() || json[version_value] != '1' ||
        (version_value + 1 < json.size() && std::isdigit(static_cast<unsigned char>(json[version_value + 1]))))
        return false;

    for (size_t profile = 0; profile < 5; ++profile)
    {
        const std::string marker = std::string("\"") + kProfiles[profile] + "\"";
        const size_t profile_key = json.find(marker);
        if (profile_key == std::string::npos) return false;
        const size_t start = json.find('{', profile_key + marker.size());
        const size_t end = start == std::string::npos ? start : matching(json, start, '{', '}');
        if (end == std::string::npos) return false;
        const std::string section = json.substr(start, end - start + 1);
        for (size_t button = 0; button < 6; ++button)
        {
            if (!parse_macro(section, kButtons[button], config[profile][button]))
            {
                // Right-column buttons are optional for old three-button
                // exports; missing entries remain empty.
                if (button < 3) return false;
                config[profile][button] = {};
            }
        }
    }
    return true;
}

std::string modifier_name(uint8_t modifier)
{
    if (modifier == kCtrl) return "Ctrl";
    if (modifier == kShift) return "Shift";
    if (modifier == kAlt) return "Alt";
    return "GUI";
}

std::string chord_json(uint8_t modifiers, uint8_t key)
{
    std::ostringstream output;
    output << "[";
    bool first = true;
    for (uint8_t bit : {kCtrl, kShift, kAlt, kGui})
    {
        if (modifiers & bit)
        {
            if (!first) output << ", ";
            output << '"' << modifier_name(bit) << '"';
            first = false;
        }
    }
    if (!first) output << ", ";
    output << '"' << static_cast<char>(key) << '"' << "]";
    return output.str();
}

bool export_config(MacropadHid &hid, const std::string &path)
{
    std::ostringstream output;
    output << "{\n  \"version\": 1,\n  \"profiles\": {\n";
    for (uint8_t profile = 0; profile < 5; ++profile)
    {
        output << "    \"" << kProfiles[profile] << "\": {\n";
        for (uint8_t button = 0; button < 6; ++button)
        {
            std::array<uint8_t, 9> reply = {};
            if (!exchange(hid, kGetButton, profile, button, nullptr, &reply))
            {
                // Older three-button firmware rejects the right-column keys;
                // preserve the first three records and leave them empty.
                if (button >= 3)
                {
                    for (uint8_t missing = button; missing < 6; ++missing)
                    {
                        output << "      \"" << kButtons[missing]
                               << "\": {\"chords\": [], \"between_chords_ms\": 0}"
                               << (missing == 5 ? "\n" : ",\n");
                    }
                    break;
                }
                return false;
            }
            output << "      \"" << kButtons[button] << "\": {\"chords\": [";
            for (uint8_t step = 0; step < reply[2]; ++step)
            {
                if (step) output << ", ";
                output << chord_json(reply[3 + step * 2], reply[4 + step * 2]);
            }
            output << "], \"between_chords_ms\": "
                   << static_cast<unsigned>(reply[7]) << "}"
                   << (button == 5 ? "\n" : ",\n");
        }
        output << "    }" << (profile == 4 ? "\n" : ",\n");
    }
    output << "  }\n}\n";
    if (!write_file(path, output.str()))
    {
        std::cerr << "Could not write " << path << "\n";
        return false;
    }
    return true;
}
} // namespace

int main(int argc, char **argv)
{
    int argument = 1;
    std::string target_uid;
    if (argument < argc && std::string(argv[argument]) == "--uid")
    {
        if (argument + 1 >= argc) {
            std::cerr << "Missing UID after --uid\n";
            return 2;
        }
        target_uid = argv[argument + 1];
        argument += 2;
    }
    if (argument >= argc || argc - argument > 2)
    {
        std::cerr << "Usage: macropad-config.exe list | [--uid HEX10] board | export <file> | import <file> | reset | bootloader\n";
        return 2;
    }

    MacropadHid hid;
    hid.configure(0x1209, 0xC55D);
    hid.set_target_uid(target_uid);
    std::string error;
    const std::string command = argv[argument];
    const bool has_file = argc - argument == 2;
    if (command == "list" && !has_file)
    {
        std::vector<MacropadBoardIdentity> identities;
        if (!hid.list_identities(identities, &error))
        {
            std::cerr << error << "\n";
            return 1;
        }
        for (const auto &identity : identities)
        {
            std::cout << (identity.variant == 2 ? "six_key" : "three_key") << " uid=";
            for (uint8_t byte : identity.uid) std::printf("%02X", byte);
            std::cout << "\n";
        }
        return 0;
    }
    if (!hid.open(&error))
    {
        std::cerr << error << "\n";
        return 1;
    }
    if (command == "export" && has_file)
    {
        return export_config(hid, argv[argument + 1]) ? 0 : 1;
    }
    if (command == "reset" && !has_file)
    {
        return exchange(hid, kResetDefaults, 0, 0, nullptr, nullptr) ? 0 : 1;
    }
    if (command == "board" && !has_file)
    {
        std::array<uint8_t, 9> reply = {};
        if (!exchange(hid, kGetBoardId, 0, 0, nullptr, &reply)) return 1;
        if (reply[2] == 2)
        {
            std::cout << "six_key uid=";
            for (unsigned i = 0; i < 5; ++i) std::printf("%02X", reply[3 + i]);
            std::cout << "\n";
            return 0;
        }
        if (reply[2] == 1)
        {
            std::cout << "three_key uid=";
            for (unsigned i = 0; i < 5; ++i) std::printf("%02X", reply[3 + i]);
            std::cout << "\n";
            return 0;
        }
        std::cerr << "Unknown board ID: " << static_cast<unsigned>(reply[2]) << "\n";
        return 1;
    }
    if (command == "bootloader" && !has_file)
    {
        if (!hid.request_bootloader(&error))
        {
            std::cerr << "Could not request bootloader: " << error << "\n";
            return 1;
        }
        return 0;
    }
    if (command == "import" && has_file)
    {
        std::array<std::array<Macro, 6>, 5> config = {};
        if (!parse_config(read_file(argv[argument + 1]), config))
        {
            std::cerr << "Invalid configuration JSON. Use an exported file as the template.\n";
            return 2;
        }
        if (!exchange(hid, kBeginUpdate, 0, 0, nullptr, nullptr)) return 1;
        for (uint8_t profile = 0; profile < 5; ++profile)
        {
            for (uint8_t button = 0; button < 6; ++button)
            {
                if (!exchange(hid, kSetButton, profile, button, &config[profile][button], nullptr))
                {
                    // A three-key firmware has no BTN_4; leave it untouched.
                    if (button == 3) continue;
                    exchange(hid, kAbort, 0, 0, nullptr, nullptr);
                    return 1;
                }
                if (!exchange(hid, kSetDelay, profile, button, &config[profile][button], nullptr))
                {
                    if (button == 3) continue;
                    exchange(hid, kAbort, 0, 0, nullptr, nullptr);
                    return 1;
                }
            }
        }
        return exchange(hid, kCommit, 0, 0, nullptr, nullptr) ? 0 : 1;
    }

    std::cerr << "Usage: macropad-config.exe list | [--uid HEX10] board | export <file> | import <file> | reset | bootloader\n";
    return 2;
}
