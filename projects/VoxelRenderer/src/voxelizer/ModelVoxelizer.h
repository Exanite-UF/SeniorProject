#pragma once

#include <src/voxelizer/Model.h>

class ModelVoxelizer
{

private:
    Model* loadedModel = nullptr; // might want to replace with shared ptr

public:
    ~ModelVoxelizer();
    void loadModel(char* path);
    Model* getModel();
};