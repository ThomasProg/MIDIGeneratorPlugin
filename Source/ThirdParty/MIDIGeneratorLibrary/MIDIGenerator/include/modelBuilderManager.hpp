#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>

#include "modelBuilderManager.h"

#include "json.hpp"
#include "fwd.h"
#include <type_traits>

#include "abstractPipeline.hpp" // have to import to show AOnnxModel inherits from AModel from covariant return
#include "typeinfo.hpp"

ASSERT_CPP_COMPILATION

class API_EXPORT ModelLoadingParamsWrapper
{
public:
    struct ModelLoadingParams& internal;

public:
    ModelLoadingParamsWrapper();
    ~ModelLoadingParamsWrapper();
    CppStr getModelType() const;
};

class API_EXPORT ModelBuilder : public Object
{
public:
    // BEGIN - Object
    static const TypeInfo& getStaticTypeInfo();
    virtual const TypeInfo& getTypeInfo() override;
    // END - Object

    virtual AModel* loadModel(const ModelLoadingParams& loadingData) const = 0;
    AModel* loadModelFromWrapper(const ModelLoadingParamsWrapper& loadingData) const;
    virtual ~ModelBuilder() = default;
};

class API_EXPORT OnnxModelBuilder : public ModelBuilder
{
public:
    Ort::Env* env = nullptr;

    // BEGIN - Object
    static const TypeInfo& getStaticTypeInfo();
    virtual const TypeInfo& getTypeInfo() override;
    // END - Object

    // BEGIN - ModelBuilder
    virtual AOnnxModel* loadModel(const ModelLoadingParams& loadingData) const override = 0;
    // END - ModelBuilder
};

// Singleton Accessor
class API_EXPORT ModelBuilderManager
{
public:
    void registerModelBuilder(const char* modelType, ModelBuilder*&& modelBuilder);
    const ModelBuilder* loadBuilder(const ModelLoadingParams& loadingData) const;
    AModel* loadModel(const ModelLoadingParams& loadingData) const;
    AModel* loadModel(const char* folderPath) const;

    ModelBuilder* findBuilder(const char* modelType) const;

    template<typename T>
    // requires std::is_base_of_v<ModelBuilder, T>
    T* findBuilder(const char* modelType) const
    {
        return dynamicCast<T>(findBuilder(modelType));
    }
};


API_EXPORT ModelBuilderManager& getModelBuilderManager();