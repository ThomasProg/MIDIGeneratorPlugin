#pragma once

#include "fwd.h"

class TokenHistory;
class GenerationHistory;

using GenerationHistoryHandle = GenerationHistory*;
using TokenHistoryHandle = TokenHistory*;

extern "C" 
{
    API_EXPORT void addEncodedToken(const GenerationHistoryHandle genHistory, int32_t newEncodedToken);

    API_EXPORT bool hadEncodedTokenRecently(const GenerationHistoryHandle genHistory, int32_t token, int32_t maxAge);
    API_EXPORT bool hadDecodedTokenRecently(const GenerationHistoryHandle genHistory, int32_t token, int32_t maxAge);

    // const, must not be modified directly, must go through this class to be modified
    API_EXPORT TokenHistoryHandle getEncodedTokensHistory(const GenerationHistoryHandle genHistory);
    API_EXPORT TokenHistoryHandle getDecodedTokensHistory(const GenerationHistoryHandle genHistory);
}

extern "C" 
{
    API_EXPORT void addToken(TokenHistoryHandle tokenHistory, int32_t newToken);
    API_EXPORT bool findMostRecentAge(TokenHistoryHandle tokenHistory, int32_t token, int32_t* outAge);
    API_EXPORT bool hadTokenRecently(TokenHistoryHandle tokenHistory, int32_t token, int32_t maxAge);
}