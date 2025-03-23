#include <src/voxelizer/ModelVoxelizer.h>

ModelVoxelizer::~ModelVoxelizer()
{
    if (loadedModel)
    {
        delete loadedModel;
    }
}

Model* ModelVoxelizer::getModel()
{
    return loadedModel;
}

void ModelVoxelizer::loadModel(char* path)
{
    loadedModel = new Model(path);
}