#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class VoxelWorld
{
private:
	GLuint makeNoiseComputeProgram;
	GLuint makeMipMapComputeProgram;
	GLuint assignMaterialComputeProgram;

	GLuint occupancyMap;
    GLuint mipMap1;
    GLuint mipMap2;
    GLuint mipMap3;
    GLuint mipMap4;

	double currentNoiseTime;

	void makeNoise(GLuint image3D, double noiseTime, bool isRand2, float fillAmount);
	void makeMipMap(GLuint inputImage3D, GLuint outputImage3D);
	void assignMaterial(GLuint image3D);

public:
	VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram);
	
	// isRand2 = Noise type toggle
	void generateFromNoise(double deltaTime, bool isRand2, float fillAmount); 
	void bindTextures();
};