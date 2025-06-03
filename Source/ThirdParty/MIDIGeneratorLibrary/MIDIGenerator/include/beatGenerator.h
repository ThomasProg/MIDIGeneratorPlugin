#pragma once

#include "note.h"

#ifdef __cplusplus
extern "C" {
#endif

enum class BeatType
{
    KICK,
    SNARE,
    HI_HAT,
    CLOSED_HAT,
    OPEN_HAT
};

typedef struct BeatNote
{
    BeatType type;
    Note note;

    BeatNote(BeatType inType, const Note& inNote) : type(inType), note(inNote) {}
} API_EXPORT BeatNote;

class BeatGenerator;
using BeatGeneratorHandle = BeatGenerator*;

extern "C"
{
    API_EXPORT BeatGeneratorHandle createBeatGenerator();
    API_EXPORT void destroyBeatGenerator(BeatGeneratorHandle beatGenerator);

    API_EXPORT void beatGenerator_getNotes(BeatGeneratorHandle beatGenerator, const BeatNote** outNotes, int32_t* outLength);
    API_EXPORT void beatGenerator_rewind(BeatGeneratorHandle beatGenerator, int tick);
    API_EXPORT void beatGenerator_refresh(BeatGeneratorHandle beatGenerator, const Note* melodyBegin, const Note* melodyEnd);
    API_EXPORT void beatGenerator_refreshFromGenHistory(BeatGeneratorHandle beatGenerator, const GenerationHistory* melodyGenHistory);
}

#ifdef __cplusplus
}  // End extern "C"
#endif 