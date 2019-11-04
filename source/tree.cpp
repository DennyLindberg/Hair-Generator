#include "tree.h"

void GenerateLeaf(Canvas2D & leafCanvas, GLTriangleMesh& leafMesh)
{
	/*
		Leaf texture
	*/
	int leafTextureSize = leafCanvas.GetTexture()->width;
	Color leafFillColor{ 0,200,0,0 };
	Color leafLineColor{ 0,100,0,255 };

	leafCanvas.Fill(leafFillColor);
	std::vector<glm::fvec3> leafHull;
	DrawFractalLeaf(
		leafHull,
		leafCanvas,
		leafLineColor,
		6,
		1.0f,
		glm::fvec2(leafTextureSize*0.5, leafTextureSize),
		90
	);

	glm::fvec2 previous = leafHull[0];
	for (int i = 1; i < leafHull.size(); i++)
	{
		leafCanvas.DrawLine(leafHull[i - 1], leafHull[i], leafLineColor);
	}
	leafCanvas.DrawLine(leafHull.back(), leafHull[0], leafLineColor);
	leafCanvas.GetTexture()->CopyToGPU();


	/*
		Create leaf mesh by converting turtle graphics to vertices and UV coordinates.
	*/
	glm::fvec3 leafNormal{ 0.0f, 0.0f, 1.0f };

	// Normalize leafHull dimensions to [0, 1.0]
	for (auto& h : leafHull)
	{
		h = h / float(leafTextureSize);
	}
	for (glm::fvec3& p : leafHull)
	{
		leafMesh.AddVertex(
			{ p.x - 0.5f, p.z, 1.0f - p.y }, // convert to world coordinate system (tip points towards x+, face towards z+)
			leafNormal,
			{ 1.0f, 0.0f, 0.0f, 1.0f }, // vertex color
			{ p.x, p.y, 0.0f, 0.0f }	// texture coordinate
		);
	}
	for (int i = 1; i < leafHull.size() - 1; i++)
	{
		leafMesh.DefineNewTriangle(0, i, i + 1);
	}
	leafMesh.ApplyMatrix(glm::scale(glm::mat4{ 1.0f }, glm::fvec3{ 0.5f }));
	leafMesh.SendToGPU();
}

void GenerateNewTree(TreeStyle style, GLLine& skeletonLines, GLTriangleMesh& branchMeshes, GLTriangleMesh& crownLeavesMeshes, const GLTriangleMesh& leafMesh, UniformRandomGenerator& uniformGenerator, int treeIterations, int treeSubdivisions)
{
	skeletonLines.Clear();
	branchMeshes.Clear();
	crownLeavesMeshes.Clear();

	/*
		Tree branch propertes
	*/
	float trunkThickness = 0.5f * powf(1.3f, float(treeIterations));
	float branchScalar = 0.4f;											// how the branch thickness relates to the parent
	float depthScalar = powf(0.75f, 1.0f / float(treeSubdivisions));	// how much the branch shrinks in thickness the farther from the root it goes (the pow is to counter the subdiv growth)
	const int trunkCylinderDivisions = 32;

	/*
		Leaf generation properties
	*/
	float leafMinScale = 0.25f;
	float leafMaxScale = 1.5f;
	float growthCurve = treeIterations / (1.0f + float(treeIterations));
	float pruningChance = growthCurve * 2.0f - 1.0f;// Random chance to remove a leaf (chance increases by the number of iterations)

	int leavesPerBranch = 25 - int(20 * (growthCurve * 2.0f - 1.0f));
	leavesPerBranch = (leavesPerBranch == 0) ? 1 : leavesPerBranch;



	/*
		Helper functions
	*/
	auto& getBranchThickness = [&](int branchDepth, int nodeDepth) -> float
	{
		return trunkThickness * powf(branchScalar, float(branchDepth)) * powf(depthScalar, float(nodeDepth));
	};

	auto& getCylinderDivisions = [&](int branchDepth) -> int
	{
		int cylinderDivisions = int(trunkCylinderDivisions / pow(2, branchDepth));
		return (cylinderDivisions < 4) ? 6 : cylinderDivisions;
	};

	int branchCount = 0;
	GenerateFractalTree3D(
		style,
		uniformGenerator,
		treeIterations,
		treeSubdivisions,
		true,
		[&](Bone<FractalTree3DProps>* root, std::vector<FractalBranch>& branches) -> void
	{
		if (!root) return;
		using TBone = Bone<FractalTree3DProps>;

		for (int b = 0; b < branches.size(); b++)
		{
			int cylinderDivisions = getCylinderDivisions(branches[b].depth);
			GLTriangleMesh newBranchMesh{ false };

			/*
				Vertex
				Positions, Normals, Texture Coordinates
			*/
			// Create vertex rings around each bone
			auto& branchNodes = branches[b].nodes;
			float rootLength = branchNodes[0]->length;
			float texU = 0.0f; // Texture coordinate along branch, it varies depending on the bone length and must be tracked
			for (int depth = 0; depth < branchNodes.size(); depth++)
			{
				auto& bone = branchNodes[depth];
				float thickness = getBranchThickness(branches[b].depth, bone->nodeDepth);
				float circumference = 2.0f*PI_f*thickness;
				texU += bone->length / circumference;

				skeletonLines.AddLine(bone->transform.position, bone->tipPosition(), glm::fvec4(0.0f, 1.0f, 0.0f, 1.0f));
				skeletonLines.AddLine(bone->transform.position, bone->transform.position+bone->transform.up*0.2f, glm::fvec4(1.0f, 0.0f, 0.0f, 1.0f));

				glm::fvec3 localX = bone->transform.up;
				glm::fvec3 localY = bone->transform.forward;

				// Make the branch root blend into its parent a bit. (this makes the branches appear less angular)
				auto& t = bone->transform;
				glm::fvec3 position = t.position;
				if (depth < (treeSubdivisions - 1) && branchNodes[0]->parent)
				{
					auto& parent = branchNodes[0]->parent;
					float blendAlpha = depth / float(treeSubdivisions);

					glm::fvec3 u = parent->transform.forward;
					glm::fvec3 v = bone->transform.position - parent->transform.position;
					float length = glm::length(v);
					v /= length;
					glm::fvec3 projectionOnParent = parent->transform.position + glm::dot(u, v) * u * length * blendAlpha;

					position = glm::mix(projectionOnParent, t.position, 0.5f + 0.5f*blendAlpha);
					thickness = glm::mix(thickness / branchScalar, thickness, 0.4f + 0.6f*blendAlpha);

					// Blend orientation of cylinder ring to give a spline
					auto& parentForward = parent->transform.forward;
					auto& boneForward = bone->transform.forward;
					localY = glm::normalize(glm::mix(parentForward, boneForward, blendAlpha));
					glm::fvec3 rotationVector = glm::normalize(glm::cross(boneForward, localY));
					float angle = glm::acos(glm::dot(boneForward, localY));
					localX = glm::rotate(glm::mat4(1.0f), angle, rotationVector) * glm::fvec4(localX, 0.0f);
				}

				// Generate the cylinder ring
				float angleStep = 360.0f / float(cylinderDivisions);
				for (int i = 0; i < cylinderDivisions; i++)
				{
					float angle = angleStep * i;
					glm::mat4 rot = glm::rotate(glm::mat4{ 1.0f }, glm::radians(angle), localY);
					glm::fvec3 normal = rot * glm::fvec4(localX, 0.0f);

					newBranchMesh.AddVertex(
						position + normal * thickness,
						normal,
						glm::fvec4{ 1.0f },
						glm::fvec4{ texU, i / float(cylinderDivisions), 1.0f, 1.0f }
					);
				}

				// Add extra set of vertices for the UV seam
				newBranchMesh.AddVertex(
					position + localX * thickness,
					localX,
					glm::fvec4{ 1.0f },
					glm::fvec4{ texU, 1.0f, 1.0f, 1.0f }
				);
			}

			// Add tip for branch
			auto& lastBone = branchNodes.back();
			newBranchMesh.AddVertex(
				lastBone->tipPosition(),
				lastBone->transform.forward,
				glm::fvec4{ 1.0f },
				glm::fvec4{ texU + lastBone->length, 0.5f, 1.0f, 1.0f }
			);



			/*
				Triangle Indices
			*/
			// Generate indices for cylinders
			int ringStep = cylinderDivisions + 1; // +1 because of UV seam
			for (int depth = 1; depth < branchNodes.size(); depth++)
			{
				int uStart = depth * ringStep;
				int lStart = uStart - ringStep;

				for (int i = 0; i < cylinderDivisions; i++)
				{
					int u = uStart + i;
					int l = lStart + i;

					newBranchMesh.DefineNewTriangle(l, l + 1, u + 1);
					newBranchMesh.DefineNewTriangle(u + 1, u, l);
				}
			}

			// Generate indices for tip
			int tipIndex = int(newBranchMesh.positions.size()) - 1;
			int lastRing = ringStep * (int(branchNodes.size()) - 1);
			for (int i = 1; i < ringStep; i++)
			{
				int ringId = lastRing + i;
				newBranchMesh.DefineNewTriangle(ringId - 1, ringId, tipIndex);
			}

			branchMeshes.AppendMesh(newBranchMesh);
		}




		/*
			Generate leaves
		*/
		int maxBranchDepth = 0;
		for (auto& branch : branches)
		{
			maxBranchDepth = (branch.depth > maxBranchDepth) ? branch.depth : maxBranchDepth;
		}

		int startDepth = maxBranchDepth - 2;
		startDepth = (startDepth > 2) ? startDepth : 2;

		for (auto& branch : branches)
		{
			if (branch.depth < startDepth) continue;

			auto& branchNodes = branch.nodes;
			int lastIndex = int(branchNodes.size() - 1);
			int startIndex = int(round(0.25f * lastIndex));
			for (int i = startIndex; i <= lastIndex; ++i)
			{
				auto& leafNode = branchNodes[i];

				glm::fvec3 nodeBegin = leafNode->transform.position;
				glm::fvec3 nodeEnd = leafNode->tipPosition();
				glm::fvec3 nodeDirection = leafNode->transform.forward;
				glm::fvec3 nodeNormal = leafNode->transform.up;

				float thickness = getBranchThickness(branch.depth, leafNode->nodeDepth);
				float circumference = 2.0f*PI_f*thickness;

				int leafId = leavesPerBranch;
				float stepSize = leafNode->length / leavesPerBranch;
				glm::fvec3 position, direction, normal;
				while (leafId > 0)
				{
					leafId--;
					if (uniformGenerator.RandomFloat() < pruningChance) continue;

					// Compute the leaf placement
					position = nodeBegin + nodeDirection * (stepSize*leafId + uniformGenerator.RandomFloat(0.0f, stepSize / 2.0f));	// spread along branch
					float angle = uniformGenerator.RandomFloat(0.0f, 2.0f*PI_f);
					direction = glm::rotate(glm::mat4{ 1.0f }, angle, nodeDirection) * glm::fvec4{ nodeNormal, 1.0f };				// random direction
					position += direction * thickness;																				// push leaf so that it starts on the branch and not inside it
					direction = glm::normalize(glm::mix(direction, nodeDirection, uniformGenerator.RandomFloat(0.3f, 0.8f)));		// blend how much the leaf is angled along the branch
					angle = uniformGenerator.RandomFloat(0.0f, 22.0f* PI_f);
					normal = glm::rotate(glm::mat4{ 1.0f }, angle, direction) * glm::fvec4{ nodeDirection, 1.0f };					// random twist

					// Insert the leaf
					crownLeavesMeshes.AppendMeshTransformed(
						leafMesh,
						glm::inverse(glm::lookAt(position, position - direction, -normal)) * glm::scale(glm::mat4{ 1.0f }, glm::fvec3{ uniformGenerator.RandomFloat(leafMinScale, leafMaxScale) })
					);
				}

				// Put a leaf at the tip of the branch
				if (i == lastIndex)
				{
					crownLeavesMeshes.AppendMeshTransformed(
						leafMesh,
						glm::inverse(glm::lookAt(nodeEnd, nodeEnd - nodeDirection, -nodeNormal)) * glm::scale(glm::mat4{ 1.0f }, glm::fvec3{ uniformGenerator.RandomFloat(leafMinScale, leafMaxScale) })
					);
				}

				// Debug orientation lines
				//skeletonLines.AddLine(nodeEnd, nodeEnd + 0.2f*nodeDirection, glm::fvec4(0.0f, 1.0f, 0.0f, 1.0f));
				//skeletonLines.AddLine(nodeEnd, nodeEnd + 0.2f*nodeNormal, glm::fvec4(1.0f, 0.0f, 0.0f, 1.0f));
			}

		}

		branchCount = int(branches.size());
	});

	branchMeshes.SendToGPU();
	crownLeavesMeshes.SendToGPU();
	skeletonLines.SendToGPU();

	int branchPolycount = int(branchMeshes.indices.size() / 3);
	int leavesPolycount = int(crownLeavesMeshes.indices.size() / 3);
	int leafPolycount = int(leafMesh.indices.size() / 3);
	int numLeaves = leavesPolycount / leafPolycount;
	printf("Done! %d branches (%d triangles), %d leaves (%d triangles)", branchCount, branchPolycount, numLeaves, leavesPolycount);
}

