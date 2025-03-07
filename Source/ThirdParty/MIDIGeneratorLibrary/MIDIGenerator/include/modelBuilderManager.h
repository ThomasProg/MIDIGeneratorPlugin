#pragma once

#include "fwd.h"
#include "utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    AModelHandle (*loadModel)(const ModelLoaderHandle loader);
} API_EXPORT CModelBuilder;

struct ModelLoadingParams;

extern "C"
{
    API_EXPORT void modelBuilderManager_registerModelBuilder(CModelBuilder* builder);
    API_EXPORT void modelBuilderManager_loadModel(const char* folderPath, AModelHandle* outModel, CResult* outResult);

    API_EXPORT CResult createModelLoadingParamsFromFolder(const char* folderPath, ModelLoadingParams* outParams);
    API_EXPORT CResult createModelLoadingParamsWrapperFromFolder(const char* folderPath, ModelLoadingParamsWrapper* outParams);
    API_EXPORT CStr modelLoadingParams_getModelType(ModelLoadingParams* params);
}

#ifdef __cplusplus
}  // End extern "C"
#endif 