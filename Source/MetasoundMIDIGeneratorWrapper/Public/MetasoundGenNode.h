// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "MetasoundNodeInterface.h"

#include "HarmonixMetasound/Common.h"

//#undef DECLARE_METASOUND_PARAM_EXTERN
//#undef DECLARE_METASOUND_PARAM_ALIAS
//
//// Redifinitation because of dll linkage
//#define DECLARE_METASOUND_PARAM_EXTERN(NAME)      \
//	METASOUNDMIDIGENERATORWRAPPER_API extern const TCHAR* NAME##Name;       \
//	METASOUNDMIDIGENERATORWRAPPER_API extern const FText NAME##Tooltip;     \
//	METASOUNDMIDIGENERATORWRAPPER_API extern const FText NAME##DisplayName;
//
//#define DECLARE_METASOUND_PARAM_ALIAS(NAME)      \
//	METASOUNDMIDIGENERATORWRAPPER_API extern const TCHAR* NAME##Name;       \
//	METASOUNDMIDIGENERATORWRAPPER_API extern const FText& NAME##Tooltip;     \
//	METASOUNDMIDIGENERATORWRAPPER_API extern const FText& NAME##DisplayName;
//

//namespace AIGen
//{
//	METASOUNDMIDIGENERATORWRAPPER_API Metasound::FNodeClassName GetClassName();
//	METASOUNDMIDIGENERATORWRAPPER_API int32 GetCurrentMajorVersion();
//
//	namespace Inputs
//	{
//		DECLARE_METASOUND_PARAM_ALIAS(Transport);
//	}
//
//	namespace Outputs
//	{
//		DECLARE_METASOUND_PARAM_ALIAS(MidiStream);
//	}
//}
