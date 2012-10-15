/****************************************************/
/* breeze Engine Scene Module  (c) Tobias Zirr 2011 */
/****************************************************/

#ifndef BE_SCENE_MESH_COMPOUND
#define BE_SCENE_MESH_COMPOUND

#include "beScene.h"
#include <beCore/beShared.h>
#include <vector>
#include <lean/smart/resource_ptr.h>

#include <beMath/beAABDef.h>

namespace beScene
{

// Prototypes
class Mesh;
class MeshCache;

/// Mesh compound.
class MeshCompound : public beCore::Resource
{
public:
	/// Subset vector.
	typedef std::vector< lean::resource_ptr<const Mesh> > mesh_vector;

private:
	mesh_vector m_subsets;

	MeshCache *m_pCache;

public:
	/// Constructor,
	BE_SCENE_API MeshCompound(MeshCache *pCache = nullptr);
	/// Constructor,
	BE_SCENE_API MeshCompound(const Mesh *const *pBegin, const Mesh *const *pEnd, MeshCache *pCache = nullptr);
	/// Destructor.
	BE_SCENE_API ~MeshCompound();

	/// Adds a subset.
	BE_SCENE_API void AddSubset(const Mesh *pMesh);

	/// Gets the number of subsets.
	LEAN_INLINE uint4 GetSubsetCount() const { return static_cast<uint4>(m_subsets.size()); }
	/// Gets a subset.
	LEAN_INLINE const Mesh* GetSubset(uint4 index) const { return (index < m_subsets.size()) ? m_subsets[index].get() : nullptr; }
	/// Gets a subset.
	LEAN_INLINE const Mesh *const * GetSubsets() const { return &m_subsets[0].get(); }

	/// Gets the mesh cache.
	LEAN_INLINE MeshCache* GetCache() const { return m_pCache; }
};

/// Computes the combined bounds of the given mesh compound.
BE_SCENE_API beMath::faab3 ComputeBounds(const Mesh *const *meshes, uint4 meshCount);
/// Computes the combined bounds of the given mesh compound.
LEAN_INLINE beMath::faab3 ComputeBounds(const MeshCompound &meshes)
{
	return ComputeBounds(meshes.GetSubsets(), meshes.GetSubsetCount());
}

} // namespace

#endif