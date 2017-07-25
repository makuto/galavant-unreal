// UE4 Procedural Mesh Generation from the Epic Wiki (https://wiki.unrealengine.com/Procedural_Mesh_Generation)
//
// forked from "Engine/Plugins/Runtime/CustomMeshComponent/Source/CustomMeshComponent/Private/CustomMeshComponent.cpp"

#include "GalavantUnreal.h"
#include "DynamicMeshBuilder.h"
#include "GeneratedMeshComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "LocalVertexFactory.h"
#include "Runtime/Engine/Public/EngineGlobals.h" // GEngine
#include "Runtime/Engine/Classes/PhysicsEngine/BodySetup.h"

/** Vertex Buffer */
class FGeneratedMeshVertexBuffer : public FVertexBuffer
{
public:
	TArray<FDynamicMeshVertex> Vertices;

	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FDynamicMeshVertex), BUF_Static, CreateInfo);
		// Copy the vertex data into the vertex buffer.
		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FDynamicMeshVertex), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData, Vertices.GetData(), Vertices.Num() * sizeof(FDynamicMeshVertex));
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
};

/** Index Buffer */
class FGeneratedMeshIndexBuffer : public FIndexBuffer
{
public:
	TArray<int32> Indices;

	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo);
		// Write the indices to the index buffer.
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
		FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};

/** Vertex Factory */
class FGeneratedMeshVertexFactory : public FLocalVertexFactory
{
public:
	FGeneratedMeshVertexFactory()
	{
	}

	/** Initialization */
	void Init(const FGeneratedMeshVertexBuffer* VertexBuffer)
	{
		// Commented out to enable building light of a level (but no backing is done for the procedural mesh itself)
		//check(!IsInRenderingThread());

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitGeneratedMeshVertexFactory,
			FGeneratedMeshVertexFactory*, VertexFactory, this,
			const FGeneratedMeshVertexBuffer*, VertexBuffer, VertexBuffer,
		{
			// Initialize the vertex factory's stream components.
			FDataType NewData;
			NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,Position,VET_Float3);
			NewData.TextureCoordinates.Add(
				FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(FDynamicMeshVertex,TextureCoordinate),sizeof(FDynamicMeshVertex),VET_Float2)
				);
			NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentX,VET_PackedNormal);
			NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentZ,VET_PackedNormal);
			NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Color, VET_Color);
			VertexFactory->SetData(NewData);
		});
	}
};



/** Scene proxy */
class FGeneratedMeshSceneProxy : public FPrimitiveSceneProxy
{
public:

	FGeneratedMeshSceneProxy(UGeneratedMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{
		// Add each triangle to the vertex/index buffer
		for(int TriIdx = 0; TriIdx<Component->GeneratedMeshTris.Num(); TriIdx++)
		{
			FGeneratedMeshTriangle& Tri = Component->GeneratedMeshTris[TriIdx];

			const FVector Edge01 = (Tri.Vertex1.Position - Tri.Vertex0.Position);
			const FVector Edge02 = (Tri.Vertex2.Position - Tri.Vertex0.Position);

			const FVector TangentX = Edge01.GetSafeNormal();
			const FVector TangentZ = (Edge02 ^ Edge01).GetSafeNormal();
			const FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal();

			FDynamicMeshVertex Vert0;
			Vert0.Position = Tri.Vertex0.Position;
			Vert0.Color = Tri.Vertex0.Color;
			Vert0.SetTangents(TangentX, TangentY, TangentZ);
			Vert0.TextureCoordinate.Set(Tri.Vertex0.U, Tri.Vertex0.V);
			int32 VIndex = VertexBuffer.Vertices.Add(Vert0);
			IndexBuffer.Indices.Add(VIndex);

			FDynamicMeshVertex Vert1;
			Vert1.Position = Tri.Vertex1.Position;
			Vert1.Color = Tri.Vertex1.Color;
			Vert1.SetTangents(TangentX, TangentY, TangentZ);
			Vert1.TextureCoordinate.Set(Tri.Vertex1.U, Tri.Vertex1.V);
			VIndex = VertexBuffer.Vertices.Add(Vert1);
			IndexBuffer.Indices.Add(VIndex);

			FDynamicMeshVertex Vert2;
			Vert2.Position = Tri.Vertex2.Position;
			Vert2.Color = Tri.Vertex2.Color;
			Vert2.SetTangents(TangentX, TangentY, TangentZ);
			Vert2.TextureCoordinate.Set(Tri.Vertex2.U, Tri.Vertex2.V);
			VIndex = VertexBuffer.Vertices.Add(Vert2);
			IndexBuffer.Indices.Add(VIndex);
		}

		// Init vertex factory
		VertexFactory.Init(&VertexBuffer);

		// Enqueue initialization of render resource
		BeginInitResource(&VertexBuffer);
		BeginInitResource(&IndexBuffer);
		BeginInitResource(&VertexFactory);

		// Grab material
		Material = Component->GetMaterial(0);
		if(Material == NULL)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	}

	virtual ~FGeneratedMeshSceneProxy()
	{
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER( STAT_GeneratedMeshSceneProxy_GetDynamicMeshElements );

		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
			);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		FMaterialRenderProxy* MaterialProxy = NULL;
		if(bWireframe)
		{
			MaterialProxy = WireframeMaterialInstance;
		}
		else
		{
			MaterialProxy = Material->GetRenderProxy(IsSelected());
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				// Draw the mesh.
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = bWireframe;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;
				BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_GeneratedMeshSceneProxy_DrawDynamicElements);

		const bool bWireframe = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy WireframeMaterialInstance(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
			);

		FMaterialRenderProxy* MaterialProxy = NULL;
		if(bWireframe)
		{
			MaterialProxy = &WireframeMaterialInstance;
		}
		else
		{
			MaterialProxy = Material->GetRenderProxy(IsSelected());
		}

		// Draw the mesh.
		FMeshBatch Mesh;
		FMeshBatchElement& BatchElement = Mesh.Elements[0];
		BatchElement.IndexBuffer = &IndexBuffer;
		Mesh.bWireframe = bWireframe;
		Mesh.VertexFactory = &VertexFactory;
		Mesh.MaterialRenderProxy = MaterialProxy;
		BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
		Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
		Mesh.Type = PT_TriangleList;
		Mesh.DepthPriorityGroup = SDPG_World;
		PDI->DrawMesh(Mesh);
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

		virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}

private:

	UMaterialInterface* Material;
	FGeneratedMeshVertexBuffer VertexBuffer;
	FGeneratedMeshIndexBuffer IndexBuffer;
	FGeneratedMeshVertexFactory VertexFactory;

	FMaterialRelevance MaterialRelevance;
};


//////////////////////////////////////////////////////////////////////////

UGeneratedMeshComponent::UGeneratedMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
}

bool UGeneratedMeshComponent::SetGeneratedMeshTriangles(const TArray<FGeneratedMeshTriangle>& Triangles)
{
	GeneratedMeshTris = Triangles;

	UpdateCollision();

	// Need to recreate scene proxy to send it over
	MarkRenderStateDirty();

	return true;
}

void UGeneratedMeshComponent::AddGeneratedMeshTriangles(const TArray<FGeneratedMeshTriangle>& Triangles)
{
	GeneratedMeshTris.Append(Triangles);

	// Need to recreate scene proxy to send it over
	MarkRenderStateDirty();
}

void  UGeneratedMeshComponent::ClearGeneratedMeshTriangles()
{
	GeneratedMeshTris.Reset();

	// Need to recreate scene proxy to send it over
	MarkRenderStateDirty();
}


FPrimitiveSceneProxy* UGeneratedMeshComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* Proxy = NULL;
	// Only if have enough triangles
	if(GeneratedMeshTris.Num() > 0)
	{
		Proxy = new FGeneratedMeshSceneProxy(this);
	}
	return Proxy;
}

int32 UGeneratedMeshComponent::GetNumMaterials() const
{
	return 1;
}


FBoxSphereBounds UGeneratedMeshComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	// Only if have enough triangles
	if(GeneratedMeshTris.Num() > 0)
	{
		// Minimum Vector: It's set to the first vertex's position initially (NULL == FVector::ZeroVector might be required and a known vertex vector has intrinsically valid values)
		FVector vecMin = GeneratedMeshTris[0].Vertex0.Position;

		// Maximum Vector: It's set to the first vertex's position initially (NULL == FVector::ZeroVector might be required and a known vertex vector has intrinsically valid values)
		FVector vecMax = GeneratedMeshTris[0].Vertex0.Position;

		// Get maximum and minimum X, Y and Z positions of vectors
		for(int32 TriIdx = 0; TriIdx < GeneratedMeshTris.Num(); TriIdx++)
		{
			vecMin.X = (vecMin.X > GeneratedMeshTris[TriIdx].Vertex0.Position.X) ? GeneratedMeshTris[TriIdx].Vertex0.Position.X : vecMin.X;
			vecMin.X = (vecMin.X > GeneratedMeshTris[TriIdx].Vertex1.Position.X) ? GeneratedMeshTris[TriIdx].Vertex1.Position.X : vecMin.X;
			vecMin.X = (vecMin.X > GeneratedMeshTris[TriIdx].Vertex2.Position.X) ? GeneratedMeshTris[TriIdx].Vertex2.Position.X : vecMin.X;

			vecMin.Y = (vecMin.Y > GeneratedMeshTris[TriIdx].Vertex0.Position.Y) ? GeneratedMeshTris[TriIdx].Vertex0.Position.Y : vecMin.Y;
			vecMin.Y = (vecMin.Y > GeneratedMeshTris[TriIdx].Vertex1.Position.Y) ? GeneratedMeshTris[TriIdx].Vertex1.Position.Y : vecMin.Y;
			vecMin.Y = (vecMin.Y > GeneratedMeshTris[TriIdx].Vertex2.Position.Y) ? GeneratedMeshTris[TriIdx].Vertex2.Position.Y : vecMin.Y;

			vecMin.Z = (vecMin.Z > GeneratedMeshTris[TriIdx].Vertex0.Position.Z) ? GeneratedMeshTris[TriIdx].Vertex0.Position.Z : vecMin.Z;
			vecMin.Z = (vecMin.Z > GeneratedMeshTris[TriIdx].Vertex1.Position.Z) ? GeneratedMeshTris[TriIdx].Vertex1.Position.Z : vecMin.Z;
			vecMin.Z = (vecMin.Z > GeneratedMeshTris[TriIdx].Vertex2.Position.Z) ? GeneratedMeshTris[TriIdx].Vertex2.Position.Z : vecMin.Z;

			vecMax.X = (vecMax.X < GeneratedMeshTris[TriIdx].Vertex0.Position.X) ? GeneratedMeshTris[TriIdx].Vertex0.Position.X : vecMax.X;
			vecMax.X = (vecMax.X < GeneratedMeshTris[TriIdx].Vertex1.Position.X) ? GeneratedMeshTris[TriIdx].Vertex1.Position.X : vecMax.X;
			vecMax.X = (vecMax.X < GeneratedMeshTris[TriIdx].Vertex2.Position.X) ? GeneratedMeshTris[TriIdx].Vertex2.Position.X : vecMax.X;

			vecMax.Y = (vecMax.Y < GeneratedMeshTris[TriIdx].Vertex0.Position.Y) ? GeneratedMeshTris[TriIdx].Vertex0.Position.Y : vecMax.Y;
			vecMax.Y = (vecMax.Y < GeneratedMeshTris[TriIdx].Vertex1.Position.Y) ? GeneratedMeshTris[TriIdx].Vertex1.Position.Y : vecMax.Y;
			vecMax.Y = (vecMax.Y < GeneratedMeshTris[TriIdx].Vertex2.Position.Y) ? GeneratedMeshTris[TriIdx].Vertex2.Position.Y : vecMax.Y;

			vecMax.Z = (vecMax.Z < GeneratedMeshTris[TriIdx].Vertex0.Position.Z) ? GeneratedMeshTris[TriIdx].Vertex0.Position.Z : vecMax.Z;
			vecMax.Z = (vecMax.Z < GeneratedMeshTris[TriIdx].Vertex1.Position.Z) ? GeneratedMeshTris[TriIdx].Vertex1.Position.Z : vecMax.Z;
			vecMax.Z = (vecMax.Z < GeneratedMeshTris[TriIdx].Vertex2.Position.Z) ? GeneratedMeshTris[TriIdx].Vertex2.Position.Z : vecMax.Z;
		}

		FVector vecOrigin = ((vecMax - vecMin) / 2) + vecMin;	/* Origin = ((Max Vertex's Vector - Min Vertex's Vector) / 2 ) + Min Vertex's Vector */
		FVector BoxPoint = vecMax - vecMin;			/* The difference between the "Maximum Vertex" and the "Minimum Vertex" is our actual Bounds Box */
		return FBoxSphereBounds(vecOrigin, BoxPoint, BoxPoint.Size()).TransformBy(LocalToWorld);
	}
	else
	{
		return FBoxSphereBounds();
	}
}


bool UGeneratedMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	FTriIndices Triangle;

	for(int32 i = 0; i<GeneratedMeshTris.Num(); i++)
	{
		const FGeneratedMeshTriangle& tri = GeneratedMeshTris[i];

		Triangle.v0 = CollisionData->Vertices.Add(tri.Vertex0.Position);
		Triangle.v1 = CollisionData->Vertices.Add(tri.Vertex1.Position);
		Triangle.v2 = CollisionData->Vertices.Add(tri.Vertex2.Position);

		CollisionData->Indices.Add(Triangle);
		CollisionData->MaterialIndices.Add(i);
	}

	CollisionData->bFlipNormals = true;

	return true;
}

bool UGeneratedMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	return (GeneratedMeshTris.Num() > 0);
}

void UGeneratedMeshComponent::UpdateBodySetup()
{
	if(ModelBodySetup == NULL)
	{
		ModelBodySetup = NewObject<UBodySetup>(this, UBodySetup::StaticClass());
		ModelBodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		ModelBodySetup->bMeshCollideAll = true;
	}
}

void UGeneratedMeshComponent::UpdateCollision()
{
	if(bPhysicsStateCreated)
	{
		DestroyPhysicsState();
		UpdateBodySetup();
		CreatePhysicsState();

		// Works in Packaged build only since UE4.5:
		ModelBodySetup->InvalidatePhysicsData();
		ModelBodySetup->CreatePhysicsMeshes();
	}
}

UBodySetup* UGeneratedMeshComponent::GetBodySetup()
{
	UpdateBodySetup();
	return ModelBodySetup;
}
