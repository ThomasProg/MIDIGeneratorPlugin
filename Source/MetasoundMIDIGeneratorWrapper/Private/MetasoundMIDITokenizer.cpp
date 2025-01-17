// Copyright Prog'z. All Rights Reserved.

#include "MetasoundMIDITokenizer.h"
#include "TokenizerAsset.h"

REGISTER_METASOUND_DATATYPE(Metasound::FMetasoundMIDITokenizer, "MIDITokenizer", Metasound::ELiteralType::UObjectProxy, UTokenizerAsset);

namespace Metasound
{

	FMetasoundMIDITokenizer::FMetasoundMIDITokenizer(const TSharedPtr<Audio::IProxyData>& InInitData)
	{
		if (InInitData.IsValid())
		{
			if (InInitData->CheckTypeCast<FTokenizerProxy>())
			{
				Proxy = MakeShared<FTokenizerProxy, ESPMode::ThreadSafe>(InInitData->GetAs<FTokenizerProxy>());
			}
		}
	}

}

