// Copyright Prog'z. All Rights Reserved.

#include "MetasoundMIDIGenerator.h"

REGISTER_METASOUND_DATATYPE(Metasound::FMIDIGeneratorZZZ, "MIDIGenerator");

namespace Metasound
{

	FMIDIGeneratorZZZ::FMIDIGeneratorZZZ(const TSharedPtr<Audio::IProxyData>& InInitData)
	{
		if (InInitData.IsValid())
		{
			if (InInitData->CheckTypeCast<FMIDIGeneratorProxy>())
			{
				Proxy = MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(InInitData->GetAs<FMIDIGeneratorProxy>());
			}
		}
	}

}

