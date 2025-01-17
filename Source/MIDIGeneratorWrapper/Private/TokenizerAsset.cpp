// Copyright Prog'z. All Rights Reserved.


#include "TokenizerAsset.h"
#include "GenThread.h"

bool FTokenizer::Load(const FString& TokenizerPath)
{
	FString FullPath = FGenThread::RelativeToAbsoluteContentPath(TokenizerPath);
	Tokenizer = createMidiTokenizer(TCHAR_TO_UTF8(*FullPath));
	return (Tokenizer == nullptr);
}

TSharedPtr<Audio::IProxyData> UTokenizerAsset::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
	return MakeShared<FTokenizerProxy, ESPMode::ThreadSafe>(this);
}