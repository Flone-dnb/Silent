#pragma once

#include <string>


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class SilentMessageColor
{
public:
    SilentMessageColor() = default;

    SilentMessageColor(bool bUserMessage)
    {
        if (bUserMessage)
        {
            sTime = "rgb(170,170,170)";
            sMessage = "white";
        }
        else
        {
            sMessage = "red";
        }
    }

    std::string sTime;
    std::string sMessage;
};
