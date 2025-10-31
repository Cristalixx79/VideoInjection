#include "utils.h"

int Utils::CalculateMostSimlarTo(std::vector<int> widths, const int needed) {
    std::vector<int> results;
    for (size_t i = 0; i < widths.size(); i++)
    {
        results.push_back(needed - widths[i]);
    }

    int minn = 1000000;
    for (size_t i = 0; i < results.size(); i++)
    {
        if (results[i] < minn && results[i] >= 0) {
            minn = results[i];
        }
    }

    return needed - minn;
}

gchar* Utils::GetCurrentTimestamp() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    gchar *timestamp = new gchar[32];
    strftime(timestamp, 32, "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return timestamp;
}