#pragma once

#include "fwd.h"
#include <cstdint>
#include <vector>
#include "note.h"


class API_EXPORT MIDIConverter
{
public:
    const MidiTokenizer* tokenizerHandle;
    RedirectorHandle redirector;

    void (*onNote)(void* data, const Note&);

    virtual void reset() = 0;
    virtual bool processToken(const int32_t* tokens, int32_t nbTokens, std::int32_t& index, void* data = nullptr) = 0;
    bool processToken(const std::vector<int32_t>& tokens, std::int32_t& index, void* data = nullptr);

    // tick is included
    virtual void unwind(int32_t tick) {}

    virtual ~MIDIConverter() = default;
};



// miditok/tokenizations/remi.py/_tokens_to_score()
class API_EXPORT REMIConverter: public MIDIConverter 
{
public:
    std::int32_t currentTick = 0;
    std::int32_t tickAtCurrentBar = 0;
    std::int32_t currentBar = -1;

    std::int32_t ticksPerBar = 32;
    std::int32_t ticksPerBeat = 8;
    std::int32_t ticksPerPos = 1;

    virtual void reset() override;
    virtual bool processToken(const int32_t* tokens, int32_t nbTokens, std::int32_t& index, void* data = nullptr) override;
};


// miditok/tokenizations/tsd.py/_tokens_to_score()
class API_EXPORT TSDConverter: public MIDIConverter 
{
public:
    // The data that can change with every processToken
    struct DynamicData
    {
        std::int32_t currentTick = 0;
        std::int32_t previousNoteEnd = 0;
    };
    std::vector<DynamicData> dynamicData;

    // @TODO : Update on tokenizer change
    std::int32_t velocityOffset = 1;
    std::int32_t durationOffset = 2;
    std::int32_t defaultVelocity = 80;
    std::int32_t defaultDuration = 4;
    std::int32_t ticks_per_beat = 0;

    TSDConverter();
    virtual void reset() override;
    virtual bool processToken(const int32_t* tokens, int32_t nbTokens, std::int32_t& index, void* data = nullptr) override;
    virtual void unwind(int32_t tick) override;
};

