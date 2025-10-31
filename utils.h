#pragma once

#include "main.h"

namespace Utils
{
    template <typename T>
    class Comparator
    {
    public:
        bool operator()(T f1, T f2)
        {
            return f1.fps < f2.fps;
        }
    };

    int CalculateMostSimlarTo(std::vector<int> widths, const int needed);
    gchar* GetCurrentTimestamp();
}
