#pragma once

// Combine a series of layers into a structure that can be generated as voxels.
// Why bother? The question is where are the layers applied and how. 
// A grid may only apply to a sub-space, and that grid may produce structures that overlay another layer (like an image creating terrain).
// Can interpret combinations
// Ex. With graph, image and pathfinder, create splines as spaces. Then ideally synthesize a grid that fits tile keys to those splines. 
// Originally was thinking tree with parents defining the where and how. 
class LayerParser 
{

};