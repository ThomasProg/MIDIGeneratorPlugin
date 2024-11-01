#pragma once

namespace Ort
{
    struct Env;
}

class MidiTokenizer;
class MusicGenerator;
class Redirector;
class Redirector;
struct Input;
class TokSequence;
using TokenSequence = TokSequence;
class MIDIConverter;

using EnvHandle = Ort::Env*;
using MidiTokenizerHandle = MidiTokenizer*;
using MusicGeneratorHandle = MusicGenerator*;
using RedirectorHandle = Redirector*;
using InputHandle = Input*;
using TokenSequenceHandle = TokenSequence*;
using MidiConverterHandle = MIDIConverter*;