#include "generation/fractals.h"

void GenerateLeaf(Canvas2D& leafCanvas, GLTriangleMesh& leafMesh);

void GenerateNewTree(TreeStyle style, GLLine& skeletonLines, GLTriangleMesh& branchMeshes, GLTriangleMesh& crownLeavesMeshes, const GLTriangleMesh& leafMesh, UniformRandomGenerator& uniformGenerator, int treeIterations = 10, int treeSubdivisions = 3);