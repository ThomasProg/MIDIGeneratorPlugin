#pragma 

#include "fwd.h"

class API_EXPORT OnAddTokensArgs
{
    GenerationHistory& history;
    int32_t encodedTokenIndex;
    int32_t newEncodedToken;

public:
    OnAddTokensArgs(GenerationHistory& inHistory, int32_t inEncodedToken);
    
    // Only decoded tokens can be added
    // Encoded tokens are used in the kv cache, so they can't be manipulated freely, or the kv cache would have to be modifed too
    void addDecodedToken(int newDecodedToken);
    const MidiTokenizer& getTokenizer() const;

    int32_t getNewEncodedToken() const
    {
        return newEncodedToken;
    }

    void* getUserData();

    template<typename T>
    T& getUserData()
    {
        void* userData = getUserData();
        assert(userData != nullptr);
        return * (T*) userData;
    }
};