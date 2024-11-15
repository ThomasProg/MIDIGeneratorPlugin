// Copyright Progz. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "MetasoundDataReference.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "IAudioProxyInitializer.h"
#include "GenThread.h"

class FMIDIGeneratorProxy;
using FMIDIGeneratorProxyPtr = TSharedPtr<FMIDIGeneratorProxy, ESPMode::ThreadSafe>;

namespace Metasound
{
	// Metasound data type that holds onto a weak ptr. Mostly used as a placeholder until we have a proper proxy type.
	class METASOUNDMIDIGENERATORWRAPPER_API FMIDIGeneratorZZZ
	{
		FMIDIGeneratorProxyPtr Proxy = MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(nullptr);

	public:

		FMIDIGeneratorZZZ() = default;
		FMIDIGeneratorZZZ(const FMIDIGeneratorZZZ&) = default;
		FMIDIGeneratorZZZ& operator=(const FMIDIGeneratorZZZ& Other) = default;

		FMIDIGeneratorZZZ(const TSharedPtr<Audio::IProxyData>& InInitData);

		const FMIDIGeneratorProxyPtr& GetProxy() const
		{
			return Proxy;
		}

	};

	DECLARE_METASOUND_DATA_REFERENCE_TYPES(FMIDIGeneratorZZZ, METASOUNDMIDIGENERATORWRAPPER_API, FMIDIGeneratorZZZTypeInfo, FMIDIGeneratorZZZReadRef, FMIDIGeneratorZZZWriteRef)
}

