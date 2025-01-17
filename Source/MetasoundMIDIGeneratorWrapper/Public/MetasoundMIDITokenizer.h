// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "MetasoundDataReference.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "IAudioProxyInitializer.h"

class FTokenizerProxy;
using FTokenizerProxyPtr = TSharedPtr<FTokenizerProxy, ESPMode::ThreadSafe>;

namespace Metasound
{
	// Metasound data type that holds onto a weak ptr. Mostly used as a placeholder until we have a proper proxy type.
	class METASOUNDMIDIGENERATORWRAPPER_API FMetasoundMIDITokenizer
	{
		FTokenizerProxyPtr Proxy = MakeShared<FTokenizerProxy, ESPMode::ThreadSafe>(MidiTokenizerHandle(nullptr));

	public:

		FMetasoundMIDITokenizer() = default;
		FMetasoundMIDITokenizer(const FMetasoundMIDITokenizer&) = default;
		FMetasoundMIDITokenizer& operator=(const FMetasoundMIDITokenizer& Other) = default;

		FMetasoundMIDITokenizer(const TSharedPtr<Audio::IProxyData>& InInitData);

		const FTokenizerProxyPtr& GetProxy() const
		{
			return Proxy;
		}

	};

	DECLARE_METASOUND_DATA_REFERENCE_TYPES(FMetasoundMIDITokenizer, METASOUNDMIDIGENERATORWRAPPER_API, FMetasoundMIDITokenizerTypeInfo, FMetasoundMIDITokenizerReadRef, FMetasoundMIDITokenizerWriteRef)
}

