/*****************************************************/
/* breeze Engine Physics Module (c) Tobias Zirr 2011 */
/*****************************************************/

#ifndef BE_PHYSICS_RIGIDDYNAMICCONTROLLER
#define BE_PHYSICS_RIGIDDYNAMICCONTROLLER

#include "bePhysics.h"
#include "beEntityController.h"
#include <beEntitySystem/beSynchronized.h>
#include <lean/smart/resource_ptr.h>
#include <beMath/beVectorDef.h>

namespace bePhysics
{

// Prototypes
class SceneController;
class RigidDynamic;
class Material;
class ShapeCompound;

/// Dynamic rigid body controller class.
class RigidDynamicController : public EntityController, public beEntitySystem::Synchronized
{
private:
	bool m_bAttached;
	bool m_bKinematic;

	beMath::fvec3 m_lastScaling;

protected:
	/// Shape
	lean::resource_ptr<const ShapeCompound> m_pShape;

	/// Material
	lean::resource_ptr<const Material> m_pMaterial;

	/// Actor.
	lean::resource_ptr<RigidDynamic> m_pActor;

public:
	/// Constructor.
	BE_PHYSICS_API RigidDynamicController(beEntitySystem::Entity *pEntity, SceneController *pScene, RigidDynamic *pActor,
		const ShapeCompound *pShape = nullptr, const Material *pMaterial = nullptr);
	/// Destructor.
	BE_PHYSICS_API ~RigidDynamicController();

	/// Sets the material.
	BE_PHYSICS_API void SetMaterial(const Material *pMaterial);
	/// Gets the material.
	LEAN_INLINE const Material* GetMaterial() const { return m_pMaterial; }

	/// Sets the velocity.
	BE_PHYSICS_API void SetVelocity(const beMath::fvec3 &v);
	/// Gets the velocity.
	BE_PHYSICS_API beMath::fvec3 GetVelocity() const;

	/// Sets the mass.
	BE_PHYSICS_API void SetMass(float mass);
	/// Sets the mass.
	BE_PHYSICS_API void SetMassAndInertia(float mass);
	/// Sets the mass.
	BE_PHYSICS_API float GetMass() const;

	/// Forces this controller into synchronization with the simulation.
	BE_PHYSICS_API void Synchronize();

	/// Synchronizes this controller with the simulation.
	BE_PHYSICS_API void Flush();
	/// Synchronizes the simulation with this controller.
	BE_PHYSICS_API void Fetch();

	/// Attaches this controller to the scene.
	BE_PHYSICS_API void Attach();
	/// Detaches this controller from the scene.
	BE_PHYSICS_API void Detach();

	/// Converts the controlled actor between non-kinematic and kinematic.
	BE_PHYSICS_API void SetKinematic(bool bKinematic);
	/// Checks if the actor is currently in kinematic mode.
	LEAN_INLINE bool IsKinematic() const { return m_bKinematic; }

	/// Gets the number of child components.
	BE_PHYSICS_API uint4 GetComponentCount() const;
	/// Gets the name of the n-th child component.
	BE_PHYSICS_API beCore::Exchange::utf8_string GetComponentName(uint4 idx) const;
	/// Gets the n-th reflected child component, nullptr if not reflected.
	BE_PHYSICS_API const ReflectedComponent* GetReflectedComponent(uint4 idx) const;

	/// Gets the type of the n-th child component.
	BE_PHYSICS_API beCore::Exchange::utf8_string GetComponentType(uint4 idx) const;
	/// Gets the n-th component.
	BE_PHYSICS_API lean::cloneable_obj<lean::any, true> GetComponent(uint4 idx) const;
	/// Returns true, if the n-th component can be replaced.
	BE_PHYSICS_API bool IsComponentReplaceable(uint4 idx) const;
	/// Sets the n-th component.
	BE_PHYSICS_API void SetComponent(uint4 idx, const lean::any &pComponent);

	/// Gets the shape.
	LEAN_INLINE const ShapeCompound* GetShape() const { return m_pShape; }
	/// Gets the actor.
	LEAN_INLINE RigidDynamic* GetActor() const { return m_pActor; }

	/// Gets the reflection properties.
	BE_PHYSICS_API static Properties GetControllerProperties();
	/// Gets the reflection properties.
	BE_PHYSICS_API Properties GetReflectionProperties() const;

	/// Gets the controller type.
	BE_PHYSICS_API static utf8_ntr GetControllerType();
	/// Gets the controller type.
	utf8_ntr GetType() const { return GetControllerType(); }
};

class ResourceManager;

/// Gets the default material for dynamic rigid actors.
BE_PHYSICS_API Material* GetRigidDynamicDefaultMaterial(ResourceManager &resources);

} // namespace

#endif