// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <string>


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class SilentMessage
{
public:
    SilentMessage() = default;

    SilentMessage(bool bUserMessage)
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
