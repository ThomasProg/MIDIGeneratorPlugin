// Fill out your copyright notice in the Description page of Project Settings.


#include "MIDIGeneratorEnv.h"
#include "GenThread.h"
#include "HarmonixMetasound/DataTypes/MidiClock.h"
#include "HarmonixMetasound/DataTypes/MusicTimeInterval.h"

void FMIDIGeneratorEnv::PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens)
{
	GenThread->PreStart(TokenizerPath, ModelPath, InTokens);
}

void FMIDIGeneratorEnv::StartGeneration()
{
	GenThread->Start();

	//if (!GenThread->HasStarted())
	//{
	//	int32 CurrentTempo = 120;
	//	int32 CurrentTimeSigNum = 4;
	//	int32 CurrentTimeSigDenom = 4;
	//	MidiFileData = HarmonixMetasound::FMidiClock::MakeClockConductorMidiData(CurrentTempo, CurrentTimeSigNum, CurrentTimeSigDenom);

	//	MidiFileData->MidiFileName = "Generated";
	//	MidiFileData->TicksPerQuarterNote = 16;
	//	FMidiTrack track;
	//	track.AddEvent(FMidiEvent(0, FMidiMsg::CreateAllNotesKill()));
	//	MidiFileData->Tracks.Add(MoveTemp(track));

	//	MidiDataProxy = MakeShared<FMidiFileProxy, ESPMode::ThreadSafe>(MidiFileData);



	//	GenThread->OnGenerated.BindLambda([this](int32 NewToken)
	//		{
	//			//TokenModifSection.Lock();
	//			//bShouldUpdateTokens = true;
	//			//NewEncodedTokens.Add(NewToken);
	//			//TokenModifSection.Unlock();
	//		});

	//	GenThread->Start();
	//}
}

void FMIDIGeneratorEnv::StopGeneration()
{

}

void UMIDIGeneratorEnv::StartGeneration()
{
	Generator->MidiGenerator->StartGeneration();
}
void UMIDIGeneratorEnv::StopGeneration()
{
	Generator->MidiGenerator->StopGeneration();
}

