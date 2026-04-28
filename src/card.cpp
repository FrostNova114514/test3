#include "card.h"

#include <sstream>

// ========== 构造函数 ==========

Card::Card()
    : color('x'), type(CardType::Number), value(-1) {}

Card::Card(char color, CardType type, int value)
    : color(color), type(type), value(value) {}

// ========== Getter / Setter ==========

char Card::getColor() const {
    return color;
}

CardType Card::getType() const {
    return type;
}

int Card::getValue() const {
    return value;
}

void Card::setColor(char c) {
    color = c;
}

// ========== 规则判断：能不能打出去 ==========
//
// 基础版 UNO 简化规则：
// 1. 牌是 Wild / WildDrawFour → 直接可以打
// 2. 颜色 == currentColor → 可以打
// 3. 类型相同 → 可以打
// 4. 两张都是数字牌且数字相同 → 可以打
// （+4 的严格“必须没有同色可出牌”规则 Phase 1 先不实现）
bool Card::isPlayable(const Card& topCard, char currentColor) const {
    // Wild / Wild Draw Four 总是可以打（Phase 1 简化）
    if (type == CardType::Wild || type == CardType::WildDrawFour) {
        return true;
    }

    // 颜色匹配当前有效颜色
    if (color == currentColor) {
        return true;
    }

    // 类型匹配顶牌
    if (type == topCard.getType()) {
        return true;
    }

    // 数字匹配顶牌
    if (type == CardType::Number &&
        topCard.getType() == CardType::Number &&
        value == topCard.getValue()) {
        return true;
    }

    return false;
}

// ========== toString：调试 / UI 展示 ==========

std::string Card::toString() const {
    std::ostringstream oss;

    // 颜色
    switch (color) {
        case 'r': oss << "R"; break;
        case 'y': oss << "Y"; break;
        case 'g': oss << "G"; break;
        case 'b': oss << "B"; break;
        case 'x': oss << "X"; break; // Wild / 无颜色
        default:  oss << "?"; break;
    }

    oss << "-";

    // 类型 + 数值
    switch (type) {
        case CardType::Number:
            oss << value;
            break;
        case CardType::Skip:
            oss << "Skip";
            break;
        case CardType::Reverse:
            oss << "Rev";
            break;
        case CardType::DrawTwo:
            oss << "+2";
            break;
        case CardType::Wild:
            oss << "Wild";
            break;
        case CardType::WildDrawFour:
            oss << "Wild+4";
            break;
    }

    return oss.str();
}
