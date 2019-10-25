#pragma once

#include <string>

class SilentMessageColor
{
public:
    SilentMessageColor() = default;

    SilentMessageColor(bool bUserMessage)
    {
        if (bUserMessage)
        {
            time = "rgb(170,170,170)";
            message = "white";
        }
        else
        {
            message = "red";
        }
    }

    std::string time;
    std::string message;
};
