#include "PreHeader.hpp"
#include "PhysicsSystem.hpp"

using namespace ECSEngine;
using namespace physx;

class SimulationCallback : public PxSimulationEventCallback
{
	vector<PxActor *> &_awakeActors;

public:
	SimulationCallback(vector<PxActor *> &awakeActors) : _awakeActors(awakeActors) {}
	virtual void onConstraintBreak(PxConstraintInfo *constraints, PxU32 count) override;
	virtual void onWake(PxActor **actors, PxU32 count) override;
	virtual void onSleep(PxActor **actors, PxU32 count) override;
	virtual void onTrigger(PxTriggerPair *pairs, PxU32 count) override;
	virtual void onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 numPairs) override;
	virtual void onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count) override;
};

struct _PXDeleter
{
	template <typename T> void operator()(T *ptr)
	{
		if (ptr)
		{
			ptr->release();
		}
	}
};

template <typename T> using PXUniquePtr = unique_ptr<T, _PXDeleter>;

struct PhysXSystem : PhysicsSystem
{
	enum class PhysXProcessingTarget { CPU, GPU };
	enum class BroadPhaseType { SAP, MBP, GPU };

	virtual void Update(Environment &env) override
	{
		if (env.timeSinceLastFrame < DefaultF32Epsilon) // skipping the very first update
		{
			return;
		}

		_physXScene->simulate(env.timeSinceLastFrame, nullptr, _simulationMemory.get(), _simulationMemorySize);
		_physXScene->fetchResults(true);

		env.messageBuilder.ComponentChangedHint(Position::Description(), _awakeActors.size());
		env.messageBuilder.ComponentChangedHint(Rotation::Description(), _awakeActors.size());

		for (const PxActor *awake : _awakeActors)
		{
			ASSUME(awake->is<PxRigidActor>());

			const PxRigidActor *rigid = static_cast<const PxRigidActor *>(awake);

			EntityID id;
			MemOps::Copy(&id, reinterpret_cast<const EntityID *>(&rigid->userData), 1);

			const auto &phyPos = rigid->getGlobalPoseWithoutActor();

			Position pos;
			pos.position = {phyPos.p.x, phyPos.p.y, phyPos.p.z};
			env.messageBuilder.ComponentChanged(id, pos);

			Rotation rot;
			rot.rotation = {phyPos.q.x, phyPos.q.y, phyPos.q.z, phyPos.q.w};
			env.messageBuilder.ComponentChanged(id, rot);
		}
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) override
	{
		for (const auto &entry : stream)
		{
			const Vector3 *scale = nullptr;
			if (auto *sc = entry.FindComponent<Scale>(); sc)
			{
				scale = &sc->scale;
			}
			const Vector3 *linearVelocity = nullptr;
			if (auto *lvc = entry.FindComponent<LinearVelocity>(); lvc)
			{
				linearVelocity = &lvc->velocity;
			}
			const Vector3 *angularVelocity = nullptr;
			if (auto *avc = entry.FindComponent<AngularVelocity>(); avc)
			{
				angularVelocity = &avc->velocity;
			}
			PhysicsPropertiesAssetId physicsPropertiesId;
			if (auto *pc = entry.FindComponent<Physics>(); pc)
			{
				physicsPropertiesId = pc->physics;
			}
			AddObject(env, entry.entityID, entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation, scale, linearVelocity, angularVelocity, entry.FindComponent<BoxCollider>(), entry.FindComponent<SphereCollider>(), entry.FindComponent<CapsuleCollider>(), entry.FindComponent<MeshCollider>(), physicsPropertiesId);
		}
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) override
	{
	}
	
	virtual void ControlInput(Environment &env, const ControlAction &action) override
	{
	}

	//optional<f32> mass // used
	//f32 linearDamping // used
	//f32 angularDamping // used
	//boolVector3 lockPositionAxis // used
	//boolVector3 lockRotationAxis // used
	//optional<Vector3> centerOfMass // used
	//optional<Vector3> inertiaTensor
	//optional<Quaternion> inertiaTensorRotation
	//optional<ui32> solverPositionIterations // used
	//optional<ui32> solverVelocityIterations // used
	//optional<f32> maxAngularVelocity // used
	//optional<f32> maxDepenetrationVelocity // used
	//optional<f32> sleepThreshold // used
	//optional<f32> wakeCounter // used
	//optional<f32> contactOffset // used
	//optional<f32> restOffset // used

	void AddObject(System::Environment &env, EntityID entityId, const Vector3 &position, const Quaternion &rotation, const Vector3 *scale, const Vector3 *linearVelocity, const Vector3 *angularVelocity, const BoxCollider *box, const SphereCollider *sphere, const CapsuleCollider *capsule, const MeshCollider *mesh, PhysicsPropertiesAssetId physicsPropertiesId)
	{
		const PhysicsProperties *physicsProperties = nullptr;

		if (physicsPropertiesId)
		{
			auto cachedPhysicsPropertiesIt = _cachedPhysicsProperties.find(physicsPropertiesId);
			if (cachedPhysicsPropertiesIt == _cachedPhysicsProperties.end())
			{
				const auto *loaded = env.assetsManager.Load<PhysicsPropertiesAsset>(physicsPropertiesId);
				if (loaded == nullptr)
				{
					SOFTBREAK;
					return;
				}
				else
				{
					cachedPhysicsPropertiesIt = _cachedPhysicsProperties.insert({physicsPropertiesId, loaded->data}).first;
				}
			}
			physicsProperties = &cachedPhysicsPropertiesIt->second;
		}

		bool isDynamic = physicsProperties != nullptr;
		f32 sleepThreshold = isDynamic && physicsProperties->sleepThreshold ? *physicsProperties->sleepThreshold : _sleepThreshold;
		f32 wakeCounter = isDynamic && physicsProperties->wakeCounter ? *physicsProperties->wakeCounter : _wakeCounter;
		f32 contactOffset = isDynamic && physicsProperties->contactOffset ? *physicsProperties->contactOffset : _contactOffset;
		f32 restOffset = isDynamic && physicsProperties->restOffset ? *physicsProperties->restOffset : _restOffset;
		ui32 solverPositionIterations = isDynamic && physicsProperties->solverPositionIterations ? *physicsProperties->solverPositionIterations : _solverPositionIterations;
		ui32 solverVelocityIterations = isDynamic && physicsProperties->solverVelocityIterations ? *physicsProperties->solverVelocityIterations : _solverVelocityIterations;
		f32 maxAngularVelocity = isDynamic && physicsProperties->maxAngularVelocity ? *physicsProperties->maxAngularVelocity : _maxAngularVelocity;
		f32 maxDepenetrationVelocity = isDynamic && physicsProperties->maxDepenetrationVelocity ? *physicsProperties->maxDepenetrationVelocity : _maxDepenetrationVelocity;

		PxShape *shape;
		PxRigidActor *actor;

		Vector3 objectScale = scale ? *scale : Vector3(1, 1, 1);

		if (isDynamic)
		{
			PxRigidDynamic *dynamicActor = _physics->createRigidDynamic(PxTransform(position.x, position.y, position.z, PxQuat{rotation.x, rotation.y, rotation.z, rotation.w}));
			dynamicActor->setSleepThreshold(sleepThreshold);
			dynamicActor->setWakeCounter(wakeCounter);

			PxRigidDynamicLockFlags lockFlags{};
			if (physicsProperties->lockPositionAxis[0]) lockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
			if (physicsProperties->lockPositionAxis[1]) lockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
			if (physicsProperties->lockPositionAxis[2]) lockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
			if (physicsProperties->lockRotationAxis[0]) lockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
			if (physicsProperties->lockRotationAxis[1]) lockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
			if (physicsProperties->lockRotationAxis[2]) lockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
			if (lockFlags)
			{
				dynamicActor->setRigidDynamicLockFlags(lockFlags);
			}

			actor = dynamicActor;
		}
		else
		{
			PxRigidStatic *staticActor = _physics->createRigidStatic(PxTransform(position.x, position.y, position.z, PxQuat{rotation.x, rotation.y, rotation.z, rotation.w}));
			actor = staticActor;
		}
		actor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);

		auto attachShape = [this, actor, contactOffset, restOffset](const PxGeometry &geometry, bool isTrigger)
		{
			PxShapeFlags flags = PxShapeFlag::eSCENE_QUERY_SHAPE;
			if (isTrigger)
			{
				flags |= PxShapeFlag::eTRIGGER_SHAPE;
			}
			else
			{
				flags |= PxShapeFlag::eSIMULATION_SHAPE;
			}
			PxShape *shape = _physics->createShape(geometry, *_physXMaterial, flags);
			actor->attachShape(*shape);
			shape->setContactOffset(contactOffset);
			shape->setRestOffset(restOffset);
			return shape;
		};

		if (box)
		{
			auto size = box->size * objectScale * 0.5f;
			auto *boxShape = attachShape(PxBoxGeometry(size.x, size.y, size.z), box->isTrigger);
			boxShape->setLocalPose(PxTransform(box->center.x, box->center.y, box->center.z, PxQuat{box->rotation.x, box->rotation.y, box->rotation.z, box->rotation.w}));
		}
		if (sphere)
		{
			auto size = sphere->radius * std::max(std::max(objectScale.x, objectScale.y), objectScale.z);
			auto *sphereShape = attachShape(PxSphereGeometry(size), sphere->isTrigger);
			sphereShape->setLocalPose(PxTransform(sphere->center.x, sphere->center.y, sphere->center.z, PxQuat{}));
		}
		if (capsule)
		{
			auto *capsuleShape = attachShape(PxCapsuleGeometry(capsule->radius, capsule->height * 0.5f), capsule->isTrigger);
			if (capsule->direction != CapsuleCollider::Direction::X)
			{
				PxQuat localRotation = capsule->direction == CapsuleCollider::Direction::Y ? PxQuat(PxHalfPi, PxVec3(0, 0, 1)) : PxQuat(PxHalfPi, PxVec3(0, 1, 0));
				capsuleShape->setLocalPose(PxTransform(localRotation));
			}
		}
		if (mesh)
		{
			const MeshAsset *meshAsset = env.assetsManager.Load<MeshAsset>(mesh->mesh);
			if (!meshAsset)
			{
				env.logger.Error("AddObject failed to load mesh asset\n");
				return;
			}

			PxConvexFlags convexFlags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eSHIFT_VERTICES;

			PxU16 vertexLimit = mesh->vertexLimit;
			if (vertexLimit < 8)
			{
				if (vertexLimit < 4)
				{
					SOFTBREAK;
					vertexLimit = 4;
				}
				convexFlags |= PxConvexFlag::ePLANE_SHIFTING;
			}

			PxConvexMeshDesc convexMeshDesc;
			convexMeshDesc.points.data = meshAsset->data.get();
			convexMeshDesc.points.count = meshAsset->desc.TotalVertexCount();
			convexMeshDesc.points.stride = meshAsset->desc.Stride();
			convexMeshDesc.flags = convexFlags;
			convexMeshDesc.vertexLimit = vertexLimit;

			// Set up cooking
			//const PxCookingParams currentParams = _cooking->getParams();
			//PxCookingParams newParams = currentParams;

			//if (!!(CookFlags & EPhysXMeshCookFlags::SuppressFaceRemapTable))
			//{
			//	newParams.suppressTriangleMeshRemapTable = true;
			//}

			//if (!!(CookFlags & EPhysXMeshCookFlags::DeformableMesh))
			//{
			//	// Meshes which can be deformed need different cooking parameters to inhibit vertex welding and add an extra skin around the collision mesh for safety.
			//	// We need to set the meshWeldTolerance to zero, even when disabling 'clean mesh' as PhysX will attempt to perform mesh cleaning anyway according to this meshWeldTolerance
			//	// if the convex hull is not well formed.
			//	// Set the skin thickness as a proportion of the overall size of the mesh as PhysX's internal tolerances also use the overall size to calculate the epsilon used.
			//	const FBox Bounds(SrcBuffer);
			//	const float MaxExtent = (Bounds.Max - Bounds.Min).Size();

			//	newParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
			//	newParams.meshWeldTolerance = 0.0f;
			//}

			// Do we want to do a 'fast' cook on this mesh, may slow down collision performance at runtime
			//if (!!(CookFlags & EPhysXMeshCookFlags::FastCook))
			//{
			//	newParams.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
			//}

			//_cooking->setParams(newParams);

			//if (bUseBuffer)
			//{
			//	// Cook the convex mesh to a temp buffer
			//	TArray<uint8> CookedMeshBuffer;
			//	FPhysXOutputStream Buffer(&CookedMeshBuffer);
			//	if (_cooking->cookConvexMesh(convexMeshDesc, Buffer))
			//	{
			//		CookResult = EPhysXCookingResult::Succeeded;
			//	}
			//	else
			//	{
			//	}

			//	if (CookedMeshBuffer.Num() == 0)
			//	{
			//		CookResult = EPhysXCookingResult::Failed;
			//	}

			//	if (CookResult != EPhysXCookingResult::Failed)
			//	{
			//		// Append the cooked data into cooked buffer
			//		OutBuffer.Append(CookedMeshBuffer);
			//	}
			//}
			//else
			//{
				PxConvexMesh *cookedMesh = _cooking->createConvexMesh(convexMeshDesc, _physics->getPhysicsInsertionCallback());
				if (!cookedMesh)
				{
					env.logger.Error("createConvexMesh failed\n");
					return;
				}
			//}

			// Return default cooking params to normal
			//_cooking->setParams(currentParams);

			attachShape(PxConvexMeshGeometry{cookedMesh, PxVec3{objectScale.x, objectScale.y, objectScale.z}}, mesh->isTrigger);
		}

		if (isDynamic)
		{
			PxRigidDynamic *dynamicActor = static_cast<PxRigidDynamic *>(actor);

			bool isCalculateFromShapes = !physicsProperties->mass && !physicsProperties->centerOfMass && !physicsProperties->inertiaTensor;

			if (isCalculateFromShapes)
			{
				bool result = PxRigidBodyExt::updateMassAndInertia(*dynamicActor, 100.0f);
				ASSUME(result);
			}

			if (linearVelocity)
			{
				dynamicActor->setLinearVelocity(PxVec3{linearVelocity->x, linearVelocity->y, linearVelocity->z});
			}
			if (angularVelocity)
			{
				dynamicActor->setAngularVelocity(PxVec3{angularVelocity->x, angularVelocity->y, angularVelocity->z});
			}

			if (auto com = physicsProperties->centerOfMass; com)
			{
				dynamicActor->setCMassLocalPose(PxTransform(com->x, com->y, com->z));
			}
			if (physicsProperties->mass)
			{
				dynamicActor->setMass(*physicsProperties->mass);
			}
			if (auto it = physicsProperties->inertiaTensor; it)
			{
				//dynamicActor->setMassSpaceInertiaTensor(PxVec3{it->x, it->y, it->z});
			}
			dynamicActor->setMaxAngularVelocity(maxAngularVelocity);
			dynamicActor->setMaxDepenetrationVelocity(maxDepenetrationVelocity);
			dynamicActor->setLinearDamping(physicsProperties->linearDamping);
			dynamicActor->setAngularDamping(physicsProperties->angularDamping);
			dynamicActor->setSolverIterationCounts(solverPositionIterations, solverVelocityIterations);
		}

		static_assert(sizeof(actor->userData) >= sizeof(EntityID));
		MemOps::Copy(reinterpret_cast<EntityID *>(&actor->userData), &entityId, 1);
		_physXScene->addActor(*actor);
	}

	PhysXSystem(const PhysicsSystemSettings &settings)
	{
		_simulationMemorySize = settings.simulationMemorySize * 64;
		_isProcessingOnGPU = settings.isProcessingOnGPU;
		_contactOffset = settings.contactOffset;
		_restOffset = settings.restOffset;
		_sleepThreshold = settings.sleepThreshold;
		_wakeCounter = settings.wakeCounter;
		_solverPositionIterations = settings.solverPositionIterations;
		_solverVelocityIterations = settings.solverVelocityIterations;
		_maxAngularVelocity = settings.maxAngularVelocity;
		_maxDepenetrationVelocity = settings.maxDepenetrationVelocity;

		_simulationMemory.reset(static_cast<byte *>(_aligned_malloc(_simulationMemorySize, 16)));

		_foundation.reset(PxCreateFoundation(PX_PHYSICS_VERSION, _defaultAllocator, _defaultErrorCallback));
		if (!_foundation)
		{
			SENDLOG(Error, PhysXSystem, "Initialization -> PxCreateFoundation failed\n");
			return;
		}

		//Pvd = PxCreatePvd(*Foundation);
		//PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
		//Pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

		_physics.reset(PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, PxTolerancesScale(), false, _pvd.get()));
		if (!_physics)
		{
			SENDLOG(Error, PhysXSystem, "Initialization -> PxCreatePhysics failed\n");
			return;
		}

		PxTolerancesScale toleranceScale;
		//toleranceScale.length = 1.0f;
		//toleranceScale.speed = 10.0f;

		PxCookingParams cookingParams(_physics->getTolerancesScale());
		cookingParams.meshWeldTolerance = 0.01f; // 1mm precision
		cookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
		cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
		//cookingParams.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
		//cookingParams.meshSizePerformanceTradeOff = 0.0f;

		_cooking.reset(PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, cookingParams));
		if (!_cooking)
		{
			SENDLOG(Error, PhysXSystem, "Initialization -> PxCreateCooking failed\n");
			return;
		}

		if (!PxInitExtensions(*_physics, _pvd.get()))
		{
			SENDLOG(Error, PhysXSystem, "Initialization -> PxInitExtensions failed\n");
			return;
		}

		_cpuDispatcher.reset(PxDefaultCpuDispatcherCreate(settings.workersThreadsCount));
		if (!_cpuDispatcher)
		{
			SENDLOG(Error, PhysXSystem, "Initialization -> PxDefaultCpuDispatcherCreate failed\n");
			return;
		}

		PxSceneDesc sceneDesc(_physics->getTolerancesScale());

		//PxSceneLimits sceneLimits;
		//sceneLimits.maxNbActors = 10'000;
		//sceneLimits.maxNbBodies = 10'000;
		//sceneLimits.maxNbDynamicShapes = 10'000;

		if (!SetSceneProcessing(sceneDesc, _isProcessingOnGPU ? PhysXProcessingTarget::GPU : PhysXProcessingTarget::CPU, _isProcessingOnGPU ? BroadPhaseType::GPU : BroadPhaseType::SAP))
		{
			SENDLOG(Error, PhysXSystem, "Initialization -> SetSceneProcessing failed\n");
			return;
		}

		auto filterShader = [](PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags &pairFlags, const void *constantBlock, PxU32 constantBlockSize) -> PxFilterFlags
		{
			pairFlags = PxPairFlag::eCONTACT_DEFAULT;
			#ifdef ENABLE_CONTACT_NOTIFICATIONS
				pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eNOTIFY_CONTACT_POINTS;
			#endif
			return PxFilterFlag::eDEFAULT;
		};

		sceneDesc.gravity = PxVec3(settings.gravity.x, settings.gravity.y, settings.gravity.z);
		sceneDesc.cpuDispatcher = _cpuDispatcher.get();
		//sceneDesc.filterShader = PxDefaultSimulationFilterShader;
		sceneDesc.filterShader = filterShader;
		sceneDesc.simulationEventCallback = &_simulationCallback;
		//sceneDesc.limits = sceneLimits;
		sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
		if (settings.enableAdditionalStabilization)
		{
			sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
		}
		sceneDesc.dynamicTreeRebuildRateHint = 100;
		_physXScene.reset(_physics->createScene(sceneDesc));
		if (!_physXScene)
		{
			SENDLOG(Error, PhysXSystem, "Initialization -> Physics->createScene failed\n");
			return;
		}

		// required by MBP
		//PxBounds3 region;
		//region.minimum = {-100.0f, 0.0f, -100.0f};
		//region.maximum = {100.0f, 200.0f, 100.0f};
		//PxBounds3 bounds[256];
		//const PxU32 nbRegions = PxBroadPhaseExt::createRegionsFromWorldBounds(bounds, region, 4);
		//for (PxU32 i = 0; i < nbRegions; ++i)
		//{
		//	PxBroadPhaseRegion bpregion;
		//	bpregion.bounds = bounds[i];
		//	bpregion.userData = reinterpret_cast<void *>(i);
		//	_physXScene->addBroadPhaseRegion(bpregion);
		//}

		PxPvdSceneClient *pvdClient = _physXScene->getScenePvdClient();
		if (pvdClient)
		{
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

		_physXMaterial.reset(_physics->createMaterial(0.6f, 0.6f, 0));

		//PhysXPlaneData.actor = PxCreatePlane(*Physics, PxPlane(0, 1, 0, 1), *PhysXMaterial);
		//PhysXScene->addActor(*PhysXPlaneData.actor);
		//PhysXPlaneData.WriteUserData(PhysXActorData::UserDataSource::SourceId::Plane, 0);

		//_defaultCubeShape.reset(_physics->createShape(PxBoxGeometry(0.5f, 0.5f, 0.5f), *_physXMaterial, false, PxShapeFlag::eSIMULATION_SHAPE));
		//_defaultCubeShape->setContactOffset(contactOffset);
		//_defaultCubeShape->setRestOffset(restOffset);

		//_defaultSphereShape.reset(_physics->createShape(PxSphereGeometry(0.5f), *_physXMaterial, false, PxShapeFlag::eSIMULATION_SHAPE));
		//_defaultSphereShape->setContactOffset(contactOffset);
		//_defaultSphereShape->setRestOffset(restOffset);

		SENDLOG(Info, PhysXSystem, "PhysX done initializing\n");
	}

	~PhysXSystem()
	{
		PxCloseExtensions();
	}

	bool SetSceneProcessing(PxSceneDesc &desc, PhysXProcessingTarget processingOn, BroadPhaseType broadPhaseType)
	{
		if (processingOn == PhysXProcessingTarget::GPU || broadPhaseType == BroadPhaseType::GPU)
		{
			/*PxU32 constraintBufferCapacity;	//!< Capacity of constraint buffer allocated in GPU global memory
			PxU32 contactBufferCapacity;	//!< Capacity of contact buffer allocated in GPU global memory
			PxU32 tempBufferCapacity;		//!< Capacity of temp buffer allocated in pinned host memory.
			PxU32 contactStreamSize;		//!< Size of contact stream buffer allocated in pinned host memory. This is double-buffered so total allocation size = 2* contactStreamCapacity * sizeof(PxContact).
			PxU32 patchStreamSize;			//!< Size of the contact patch stream buffer allocated in pinned host memory. This is double-buffered so total allocation size = 2 * patchStreamCapacity * sizeof(PxContactPatch).
			PxU32 forceStreamCapacity;		//!< Capacity of force buffer allocated in pinned host memory.
			PxU32 heapCapacity;				//!< Initial capacity of the GPU and pinned host memory heaps. Additional memory will be allocated if more memory is required.
			PxU32 foundLostPairsCapacity;	//!< Capacity of found and lost buffers allocated in GPU global memory. This is used for the found/lost pair reports in the BP. */

			PxgDynamicsMemoryConfig mc;
			mc.constraintBufferCapacity *= 3;
			mc.contactBufferCapacity *= 3;
			mc.tempBufferCapacity *= 3;
			mc.contactStreamSize *= 3;
			mc.patchStreamSize *= 3;
			mc.forceStreamCapacity *= 8;
			mc.heapCapacity *= 3;
			mc.foundLostPairsCapacity *= 3;
			desc.gpuDynamicsConfig = mc;

			PxCudaContextManagerDesc cudaContextManagerDesc;
			//cudaContextManagerDesc.interopMode = PxCudaInteropMode::D3D11_INTEROP;
			//cudaContextManagerDesc.graphicsDevice = Application::GetRenderer().RendererContext();
			//cudaContextManagerDesc.graphicsDevice = hg;
			_cudaContexManager.reset(PxCreateCudaContextManager(*_foundation, cudaContextManagerDesc));
			if (!_cudaContexManager || !_cudaContexManager->contextIsValid())
			{
				SENDLOG(Error, PhysXSystem, "Initialization -> PxCreateCudaContextManager failed, cannot use GPU for processing\n");
				if (processingOn == PhysXProcessingTarget::GPU)
				{
					processingOn = PhysXProcessingTarget::CPU;
				}
				if (broadPhaseType == BroadPhaseType::GPU)
				{
					broadPhaseType = BroadPhaseType::SAP;
				}
			}
			else
			{
				desc.cudaContextManager = _cudaContexManager.get();
			}
		}

		switch (broadPhaseType)
		{
		case BroadPhaseType::MBP:
		{
			desc.broadPhaseType = PxBroadPhaseType::eMBP;
		} break;
		case BroadPhaseType::SAP:
		{
			desc.broadPhaseType = PxBroadPhaseType::eSAP;
		} break;
		case BroadPhaseType::GPU:
		{
			desc.broadPhaseType = PxBroadPhaseType::eGPU;
		} break;
		}

		switch (processingOn)
		{
		case PhysXProcessingTarget::GPU:
		{
			desc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		} break;
		case PhysXProcessingTarget::CPU:
		{
		} break;
		}

		return true;
	}
	
private:
	ui32 _simulationMemorySize{};
	bool _isProcessingOnGPU{};
	f32 _contactOffset{};
	f32 _restOffset{};
	f32 _sleepThreshold{};
	f32 _wakeCounter{};
	ui32 _solverPositionIterations{};
	ui32 _solverVelocityIterations{};
	f32 _maxAngularVelocity{};
	f32 _maxDepenetrationVelocity{};

	std::unordered_map<PhysicsPropertiesAssetId, PhysicsProperties> _cachedPhysicsProperties{};
	vector<PxActor *> _awakeActors{};

	PxDefaultAllocator _defaultAllocator{};
	PxDefaultErrorCallback _defaultErrorCallback{};
	PXUniquePtr<PxFoundation> _foundation{};
	PXUniquePtr<PxPvd> _pvd{};
	PXUniquePtr<PxPhysics> _physics{};
	PXUniquePtr<PxCooking> _cooking{};
	PXUniquePtr<PxCudaContextManager> _cudaContexManager{};
	PXUniquePtr<PxDefaultCpuDispatcher> _cpuDispatcher{};
	PXUniquePtr<PxScene> _physXScene{};
	PXUniquePtr<PxMaterial> _physXMaterial{};
	//PXUniquePtr<PxShape> _defaultCubeShape{};
	//PXUniquePtr<PxShape> _defaultSphereShape{};
	unique_ptr<byte, AlignedMallocDeleter> _simulationMemory{};
	SimulationCallback _simulationCallback{_awakeActors};
};

void SimulationCallback::onConstraintBreak(PxConstraintInfo *constraints, PxU32 count)
{}

void SimulationCallback::onWake(PxActor **actors, PxU32 count)
{
	for (PxU32 index = 0; index < count; ++index)
	{
		PxActor *actor = actors[index];
		#ifdef DEBUG
			auto it = std::find(_awakeActors.begin(), _awakeActors.end(), actor);
			ASSUME(it == _awakeActors.end());
		#endif
		_awakeActors.push_back(actor);
	}
}

void SimulationCallback::onSleep(PxActor **actors, PxU32 count)
{
	for (PxU32 index = 0; index < count; ++index)
	{
		PxActor *actor = actors[index];
		auto it = std::find(_awakeActors.begin(), _awakeActors.end(), actor);
		ASSUME(it != _awakeActors.end());
		*it = _awakeActors.back();
		_awakeActors.pop_back();
	}
}

void SimulationCallback::onTrigger(PxTriggerPair *pairs, PxU32 count)
{}

void SimulationCallback::onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 numPairs)
{
//	if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
//	{
//		return;
//	}
//
//	PxRigidActor *actor0 = pairHeader.actors[0];
//	PxRigidActor *actor1 = pairHeader.actors[1];
//
//	auto &actor0data = PhysXActorData::FromUserData(*actor0);
//	auto &actor1data = PhysXActorData::FromUserData(*actor1);
//
//	auto getNextContactPairPoint = [](const PxContactPair &pair, PxContactStreamIterator &iter, uiw &count) -> optional<PxContactPairPoint>
//	{
//		const PxU32 flippedContacts = (pair.flags & PxContactPairFlag::eINTERNAL_CONTACTS_ARE_FLIPPED);
//		const PxU32 hasImpulses = (pair.flags & PxContactPairFlag::eINTERNAL_HAS_IMPULSES);
//
//		if (iter.hasNextPatch())
//		{
//			iter.nextPatch();
//			if (iter.hasNextContact())
//			{
//				iter.nextContact();
//				PxContactPairPoint dst;
//				dst.position = iter.getContactPoint();
//				dst.separation = iter.getSeparation();
//				dst.normal = iter.getContactNormal();
//				if (!flippedContacts)
//				{
//					dst.internalFaceIndex0 = iter.getFaceIndex0();
//					dst.internalFaceIndex1 = iter.getFaceIndex1();
//				}
//				else
//				{
//					dst.internalFaceIndex0 = iter.getFaceIndex1();
//					dst.internalFaceIndex1 = iter.getFaceIndex0();
//				}
//
//				if (hasImpulses)
//				{
//					const PxReal impulse = pair.contactImpulses[count];
//					dst.impulse = dst.normal * impulse;
//				}
//				else
//				{
//					dst.impulse = PxVec3(0.0f);
//				}
//
//				++count;
//
//				return dst;
//			}
//		}
//
//		return {};
//	};
//
//	for (PxU32 pairIndex = 0; pairIndex < numPairs; ++pairIndex)
//	{
//		const auto &pair = pairs[pairIndex];
//
//		if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
//		{
//			if (actor0data.actor && actor1data.actor)
//			{
//				uiw count = 0;
//				PxContactStreamIterator iter(pair.contactPatches, pair.contactPoints, pair.getInternalFaceIndices(), pair.patchCount, pair.contactCount);
//				for (auto contactPoint = getNextContactPairPoint(pair, iter, count); contactPoint; contactPoint = getNextContactPairPoint(pair, iter, count))
//				{
//					++actor0data.contactsCount;
//					++actor1data.contactsCount;
//
//#ifdef USE_XAUDIO
//					NewContactInfos.push_back({Vector3{contactPoint->position.x, contactPoint->position.y, contactPoint->position.z}, contactPoint->impulse.magnitude()});
//#endif
//				}
//			}
//		}
//		else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
//		{
//			if (actor0data.actor && actor1data.actor)
//			{
//				uiw count = 0;
//				PxContactStreamIterator iter(pair.contactPatches, pair.contactPoints, pair.getInternalFaceIndices(), pair.patchCount, pair.contactCount);
//				for (auto contactPoint = getNextContactPairPoint(pair, iter, count); contactPoint; contactPoint = getNextContactPairPoint(pair, iter, count))
//				{
//					assert(actor0data.contactsCount && actor1data.contactsCount);
//					--actor0data.contactsCount;
//					--actor1data.contactsCount;
//				}
//			}
//		}
//	}
}

void SimulationCallback::onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count)
{}

unique_ptr<PhysicsSystem> PhysicsSystem::New(const PhysicsSystemSettings &settings)
{
	return make_unique<PhysXSystem>(settings);
}