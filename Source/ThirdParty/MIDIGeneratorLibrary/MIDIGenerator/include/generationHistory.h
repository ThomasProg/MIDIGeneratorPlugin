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
    
    API_EXPORT void generationHistory_removeAfterTick(const GenerationHistoryHandle genHistory, int32_t tick);
    API_EXPORT void generationHistory_convert(const GenerationHistoryHandle genHistory);
    API_EXPORT void generationHistory_getNotes(const GenerationHistoryHandle genHistory, const struct Note** outNotes, size_t* outLength);
    API_EXPORT void generationHistory_getNotesMut(const GenerationHistoryHandle genHistory, struct Note** outNotes, size_t* outLength);
    API_EXPORT void generationHistory_addStandaloneNote(const GenerationHistoryHandle genHistory, struct Note* inNote);

    typedef void (*TOnEncodedTokenAdded)(class OnAddTokensArgs* args);
    API_EXPORT void generationHistory_setOnEncodedTokenAdded(const GenerationHistoryHandle genHistory, TOnEncodedTokenAdded inOnEncodedTokenAdd);
    API_EXPORT TOnEncodedTokenAdded generationHistory_getDefaultOnEncodedTokenAdded();
    API_EXPORT void generationHistory_setOnEncodedTokenAddedData(const GenerationHistoryHandle genHistory, void* inOnEncodedTokenAddData);

    typedef void (*TOnNoteAdded)(void* userData);
    API_EXPORT void generationHistory_setOnNoteAdded(GenerationHistory* genHistory, TOnNoteAdded inOnEncodedTokenAdd);
    API_EXPORT void generationHistory_setOnNoteAddedData(GenerationHistory* genHistory, void* inOnEncodedTokenAddData);
}

extern "C" 
{
    API_EXPORT void addToken(TokenHistoryHandle tokenHistory, int32_t newToken);
    API_EXPORT bool findMostRecentAge(TokenHistoryHandle tokenHistory, int32_t token, int32_t* outAge);
    API_EXPORT bool hadTokenRecently(TokenHistoryHandle tokenHistory, int32_t token, int32_t maxAge);
    API_EXPORT void tokenHistory_getTokens(TokenHistoryHandle tokenHistory, const int32_t** outTokens, int32_t* outSize);
}