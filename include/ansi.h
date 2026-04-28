#ifndef ANSI_H
#define ANSI_H

#include "card.h"

#include <cctype>
#include <string>

namespace ansi {
inline constexpr const char* reset() { return "\033[0m"; }
inline constexpr const char* bold() { return "\033[1m"; }
inline constexpr const char* dim() { return "\033[2m"; }

// UNO （truecolor）
// Red    #FF0000  (255, 0, 0)
// Yellow #FFFF00  (255, 255, 0)
// Green  #00AA00  (0, 170, 0)
// Blue   #0066CC  (0, 102, 204)
inline constexpr const char* fgRed() { return "\033[38;2;255;0;0m"; }
inline constexpr const char* fgYellow() { return "\033[38;2;255;255;0m"; }
inline constexpr const char* fgGreen() { return "\033[38;2;0;170;0m"; }
inline constexpr const char* fgBlue() { return "\033[38;2;0;102;204m"; }
inline constexpr const char* fgWhite() { return "\033[37m"; }
inline constexpr const char* fgMagenta() { return "\033[35m"; } // Wild

inline std::string colorForCardChar(char c) {
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(c)))) {
        case 'r': return fgRed();
        case 'y': return fgYellow();
        case 'g': return fgGreen();
        case 'b': return fgBlue();
        case 'x': return fgMagenta();
        default:  return fgWhite();
    }
}

inline std::string wildChips() {
    return std::string("[") + bold() + fgRed() + "R" + reset() + "]" +
           std::string("[") + bold() + fgYellow() + "Y" + reset() + "]" +
           std::string("[") + bold() + fgGreen() + "G" + reset() + "]" +
           std::string("[") + bold() + fgBlue() + "B" + reset() + "]";
}

inline std::string cardText(const Card& card) {
    std::string s = card.toString();
    std::string out = std::string(bold()) + colorForCardChar(card.getColor()) + s + reset();
    if (card.getType() == CardType::Wild || card.getType() == CardType::WildDrawFour) {
        out += "  " + wildChips();
    }
    return out;
}

inline std::string colorName(char color, const std::string& name) {
    return std::string(bold()) + colorForCardChar(color) + name + reset();
}
} // namespace ansi

#endif

