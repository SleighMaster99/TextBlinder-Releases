// SPDX-License-Identifier: GPL-3.0-only
#include "Color.h"

namespace tb
{
namespace
{
int HexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
}

int ParseMaskColor(std::string_view text)
{
    if (text.size() != 7 || text[0] != '#')
        return kDefaultMaskColor;

    int rgb = 0;
    for (size_t i = 1; i < text.size(); ++i)
    {
        const int d = HexDigit(text[i]);
        if (d < 0)
            return kDefaultMaskColor;
        rgb = (rgb << 4) | d;
    }

    const int r = (rgb >> 16) & 0xFF;
    const int g = (rgb >> 8) & 0xFF;
    const int b = rgb & 0xFF;
    return (b << 16) | (g << 8) | r;
}
}
