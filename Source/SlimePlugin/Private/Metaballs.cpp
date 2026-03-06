
#include "SlimePluginPrivatePCH.h"
#include "CMarchingCubes.h"
#include "Metaballs.h"





// Sets default values
AMetaballs::AMetaballs(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// Auto-possess Player 0 so this pawn is immediately playable when placed in a level
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Capsule as physics root — provides gravity, collision with floors and walls
	RootCapsule = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("RootCapsule"));
	RootCapsule->InitCapsuleSize(80.0f, 80.0f);
	RootCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RootCapsule->SetCollisionResponseToAllChannels(ECR_Block);
	RootCapsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	RootCapsule->SetSimulatePhysics(true);
	RootCapsule->SetLinearDamping(1.5f);
	RootCapsule->SetAngularDamping(10.0f); // prevents the physics body from spinning
	RootComponent = RootCapsule;

	MetaBallsBoundBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("GridBox"));
	MetaBallsBoundBox->InitBoxExtent(FVector(100, 100, 100));
	MetaBallsBoundBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MetaBallsBoundBox->SetupAttachment(RootComponent);

	// Procedural mesh is purely visual — collision is handled by RootCapsule
	m_mesh = ObjectInitializer.CreateDefaultSubobject<UProceduralMeshComponent>(this, TEXT("MetaballsMesh"));
	m_mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	m_mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	m_mesh->SetupAttachment(RootComponent);

	m_Scale = 100.0f;
	m_NumBalls = 8;
	m_automode = false; // player-controlled by default; set to true for decorative auto-fly
	m_GridStep = AMetaballs::MIN_GRID_STEPS;
	m_randomseed = false;
	m_AutoLimitX = 1.0f;
	m_AutoLimitY = 1.0f;
	m_AutoLimitZ = 1.0f;
	m_Material = nullptr;

	m_MoveSpeed           = 500.0f;
	m_SpringStiffness     = 20.0f;
	m_SpringDamping       = 5.0f;
	m_ImpactSpreadStrength = 0.5f;
	m_SpreadDecayRate     = 3.0f;

	m_pfGridEnergy     = nullptr;
	m_pnGridPointStatus = nullptr;
	m_pnGridVoxelStatus = nullptr;

	m_SpreadAmount = 1.0f;
}

void AMetaballs::PostInitializeComponents()
{

	Super::PostInitializeComponents();


	m_fLevel = 100.0f;

	m_nGridSize = 0;

	m_nMaxOpenVoxels = AMetaballs::MAX_OPEN_VOXELS;
	m_pOpenVoxels = new int[m_nMaxOpenVoxels * 3];

	m_nNumOpenVoxels = 0;
	m_pfGridEnergy = 0;
	m_pnGridPointStatus = 0;
	m_pnGridVoxelStatus = 0;

	m_nNumVertices = 0;
	m_nNumIndices = 0;

	InitBalls();

	CMarchingCubes::BuildTables();
	SetGridSize(m_GridStep);

//	m_SceneComponent->SetMobility(EComponentMobility::Movable);

	MetaBallsBoundBox->SetBoxExtent(FVector(m_Scale, m_Scale, m_Scale), false);
	MetaBallsBoundBox->UpdateBodySetup();

	// Size the collision capsule to match the visual slime radius
	float capsuleSize = m_Scale * 0.8f;
	RootCapsule->SetCapsuleSize(capsuleSize, capsuleSize);

	m_mesh->SetMaterial(1, m_Material);


}


DEFINE_LOG_CATEGORY(YourLog);

#if WITH_EDITOR
void AMetaballs::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{


	UE_LOG(YourLog, Warning, TEXT("changed respond"));

	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;

	/// track Number of balls value
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMetaballs, m_NumBalls))
	{
		FIntProperty* prop = CastField<FIntProperty>(e.Property);

		int32 value = prop->GetPropertyValue(prop->ContainerPtrToValuePtr<int32>(this));

		SetNumBalls(value);

		if (value < 0 && value > AMetaballs::MAX_METABALLS)
		{
			prop->SetPropertyValue(prop->ContainerPtrToValuePtr<int32>(this), m_NumBalls);
		}

		UE_LOG(YourLog, Warning, TEXT("Num balls value: %d"), m_NumBalls);

	}



	/// track Scale value
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMetaballs, m_Scale))
	{

		FFloatProperty* prop = CastField<FFloatProperty>(e.Property);

		float value = prop->GetPropertyValue(prop->ContainerPtrToValuePtr<float>(this));

		SetScale(value);

		if (value < AMetaballs::MIN_SCALE)
		{
			prop->SetPropertyValue(prop->ContainerPtrToValuePtr<float>(this), m_Scale);
		}

		MetaBallsBoundBox->SetBoxExtent(FVector(m_Scale, m_Scale, m_Scale), false);
		MetaBallsBoundBox->UpdateBodySetup();

		UE_LOG(YourLog, Warning, TEXT("Scale value: %f"), m_Scale);

	}


	/// track Grid steps
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMetaballs, m_GridStep))
	{

		FIntProperty* prop = CastField<FIntProperty>(e.Property);

		int32 value = prop->GetPropertyValue(prop->ContainerPtrToValuePtr<int32>(this));

		SetGridSteps(value);

		if (value < AMetaballs::MIN_GRID_STEPS && value > AMetaballs::MAX_GRID_STEPS)
		{
			prop->SetPropertyValue(prop->ContainerPtrToValuePtr<int32>(this), m_GridStep);
		}

		UE_LOG(YourLog, Warning, TEXT("Grid steps  value: %d"), m_GridStep);
	}


	/// track LimitX value
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMetaballs, m_AutoLimitX))
	{

		FFloatProperty* prop = CastField<FFloatProperty>(e.Property);

		float value = prop->GetPropertyValue(prop->ContainerPtrToValuePtr<float>(this));

		SetAutoLimitX(value);

		if (value < AMetaballs::MIN_LIMIT && value < AMetaballs::MAX_LIMIT)
		{
			prop->SetPropertyValue(prop->ContainerPtrToValuePtr<float>(this), m_AutoLimitX);
		}

//		UE_LOG(YourLog, Warning, TEXT("LimitX value: %f"), m_AutoLimitX);

	}

	/// track LimitY value
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMetaballs, m_AutoLimitY))
	{

		FFloatProperty* prop = CastField<FFloatProperty>(e.Property);

		float value = prop->GetPropertyValue(prop->ContainerPtrToValuePtr<float>(this));

		SetAutoLimitY(value);

		if (value < AMetaballs::MIN_LIMIT && value < AMetaballs::MAX_LIMIT)
		{
			prop->SetPropertyValue(prop->ContainerPtrToValuePtr<float>(this), m_AutoLimitY);
		}

				UE_LOG(YourLog, Warning, TEXT("LimitY value: %f"), m_AutoLimitY);

	}


	/// track LimitZ value
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMetaballs, m_AutoLimitZ))
	{

		FFloatProperty* prop = CastField<FFloatProperty>(e.Property);

		float value = prop->GetPropertyValue(prop->ContainerPtrToValuePtr<float>(this));

		SetAutoLimitZ(value);

		if (value < AMetaballs::MIN_LIMIT && value < AMetaballs::MAX_LIMIT)
		{
			prop->SetPropertyValue(prop->ContainerPtrToValuePtr<float>(this), m_AutoLimitZ);
		}

		UE_LOG(YourLog, Warning, TEXT("LimitZ value: %f"), m_AutoLimitZ);

	}

	Super::PostEditChangeProperty(e);

}
#endif

// Called when the game starts or when spawned
void AMetaballs::BeginPlay()
{
	Super::BeginPlay();
}

void AMetaballs::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Requires "MoveForward" and "MoveRight" axis mappings in Project Settings > Input
	PlayerInputComponent->BindAxis("MoveForward", this, &AMetaballs::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight",   this, &AMetaballs::MoveRight);
}

void AMetaballs::MoveForward(float Value)
{
	if (Value == 0.0f || !RootCapsule) return;

	FVector Dir = FVector::ForwardVector;
	if (AController* Ctrl = GetController())
	{
		FRotator Rot = Ctrl->GetControlRotation();
		Rot.Pitch = 0.0f;
		Rot.Roll  = 0.0f;
		Dir = FRotationMatrix(Rot).GetUnitAxis(EAxis::X);
	}
	// bAccelChange=true makes force mass-independent (feels the same regardless of physics mass setting)
	RootCapsule->AddForce(Dir * Value * m_MoveSpeed, NAME_None, true);
}

void AMetaballs::MoveRight(float Value)
{
	if (Value == 0.0f || !RootCapsule) return;

	FVector Dir = FVector::RightVector;
	if (AController* Ctrl = GetController())
	{
		FRotator Rot = Ctrl->GetControlRotation();
		Rot.Pitch = 0.0f;
		Rot.Roll  = 0.0f;
		Dir = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);
	}
	RootCapsule->AddForce(Dir * Value * m_MoveSpeed, NAME_None, true);
}

void AMetaballs::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
                            bool bSelfMoved, FVector HitLocation, FVector HitNormal,
                            FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Scale spread by how hard we hit (NormalImpulse is in kg*cm/s)
	float ImpactStrength = FMath::Clamp(NormalImpulse.Size() / 2000.0f, 0.05f, 1.0f);
	m_SpreadAmount = FMath::Clamp(m_SpreadAmount + m_ImpactSpreadStrength * ImpactStrength, 1.0f, 2.5f);
}

// Called every frame
void AMetaballs::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (m_NumBalls > 0)
	{
		Update(DeltaTime);
		Render();
	}

}

void AMetaballs::Update(float dt)
{
	// Decay spread back toward 1.0 (normal) over time
	m_SpreadAmount = FMath::FInterpTo(m_SpreadAmount, 1.0f, dt, m_SpreadDecayRate);

	if (IsPlayerControlled())
	{
		// --- Player-controlled slime: spring physics ---
		// Ball 0 = core, always at the actor origin in normalized space
		m_Balls[0].p = FVector::ZeroVector;
		m_Balls[0].v = FVector::ZeroVector;

		// Satellite balls (1..N-1) spring toward their rest positions,
		// scaled by m_SpreadAmount so they spread out on impact then recover
		for (int i = 1; i < m_NumBalls; i++)
		{
			FVector target      = m_Balls[i].rest * m_SpreadAmount;
			FVector displacement = m_Balls[i].p - target;
			FVector force       = -m_SpringStiffness * displacement
			                      - m_SpringDamping   * m_Balls[i].v;
			m_Balls[i].v += force * dt;
			m_Balls[i].p += m_Balls[i].v * dt;
		}
	}
	else if (m_automode)
	{
		// --- Decorative auto-fly mode (original behavior) ---
		for (int i = 0; i < m_NumBalls; i++)
		{
			m_Balls[i].p.X += dt * m_Balls[i].v.X;
			m_Balls[i].p.Y += dt * m_Balls[i].v.Y;
			m_Balls[i].p.Z += dt * m_Balls[i].v.Z;

			m_Balls[i].t -= dt;
			if (m_Balls[i].t < 0)
			{
				m_Balls[i].t   = float(rand()) / RAND_MAX;
				m_Balls[i].a.X = m_AutoLimitY * (float(rand()) / RAND_MAX * 2 - 1);
				m_Balls[i].a.Y = m_AutoLimitX * (float(rand()) / RAND_MAX * 2 - 1);
				m_Balls[i].a.Z = m_AutoLimitZ * (float(rand()) / RAND_MAX * 2 - 1);
			}

			float x = m_Balls[i].a.X - m_Balls[i].p.X;
			float y = m_Balls[i].a.Y - m_Balls[i].p.Y;
			float z = m_Balls[i].a.Z - m_Balls[i].p.Z;
			float fDist = 1 / sqrtf(x*x + y*y + z*z);
			x *= fDist; y *= fDist; z *= fDist;

			m_Balls[i].v.X += 0.1f * x * dt;
			m_Balls[i].v.Y += 0.1f * y * dt;
			m_Balls[i].v.Z += 0.1f * z * dt;

			fDist = m_Balls[i].v.X * m_Balls[i].v.X
			      + m_Balls[i].v.Y * m_Balls[i].v.Y
			      + m_Balls[i].v.Z * m_Balls[i].v.Z;
			if (fDist > 0.040f)
			{
				fDist = 1 / sqrtf(fDist);
				m_Balls[i].v.X = 0.20f * m_Balls[i].v.X * fDist;
				m_Balls[i].v.Y = 0.20f * m_Balls[i].v.Y * fDist;
				m_Balls[i].v.Z = 0.20f * m_Balls[i].v.Z * fDist;
			}

			if (m_Balls[i].p.X < -m_AutoLimitY + m_fVoxelSize) { m_Balls[i].p.X = -m_AutoLimitY + m_fVoxelSize; m_Balls[i].v.X = 0; }
			if (m_Balls[i].p.X >  m_AutoLimitY - m_fVoxelSize) { m_Balls[i].p.X =  m_AutoLimitY - m_fVoxelSize; m_Balls[i].v.X = 0; }
			if (m_Balls[i].p.Y < -m_AutoLimitX + m_fVoxelSize) { m_Balls[i].p.Y = -m_AutoLimitX + m_fVoxelSize; m_Balls[i].v.Y = 0; }
			if (m_Balls[i].p.Y >  m_AutoLimitX - m_fVoxelSize) { m_Balls[i].p.Y =  m_AutoLimitX - m_fVoxelSize; m_Balls[i].v.Y = 0; }
			if (m_Balls[i].p.Z < -m_AutoLimitZ + m_fVoxelSize) { m_Balls[i].p.Z = -m_AutoLimitZ + m_fVoxelSize; m_Balls[i].v.Z = 0; }
			if (m_Balls[i].p.Z >  m_AutoLimitZ - m_fVoxelSize) { m_Balls[i].p.Z =  m_AutoLimitZ - m_fVoxelSize; m_Balls[i].v.Z = 0; }
		}
	}
}

void AMetaballs::Render()
{

	

	m_vertices.Empty();
	m_Triangles.Empty();
	m_normals.Empty();
	m_UV0.Empty();
	m_tangents.Empty();

	m_mesh->ClearAllMeshSections();

	int nCase, x, y, z;
	bool bComputed;

	m_nNumIndices = 0;
	m_nNumVertices = 0;
	nCase = 0;

	// Clear status grids
	memset(m_pnGridPointStatus, 0, (m_nGridSize + 1)*(m_nGridSize + 1)*(m_nGridSize + 1));
	memset(m_pnGridVoxelStatus, 0, m_nGridSize*m_nGridSize*m_nGridSize);



	for (int i = 0; i < m_NumBalls; i++)
	{
		x = ConvertWorldCoordinateToGridPoint(m_Balls[i].p[0]);
		y = ConvertWorldCoordinateToGridPoint(m_Balls[i].p[1]);
		z = ConvertWorldCoordinateToGridPoint(m_Balls[i].p[2]);

		bComputed = false;

		while (1)
		{
			if (IsGridVoxelComputed(x, y, z))
			{
				bComputed = true;
				break;
			}

			nCase = ComputeGridVoxel(x, y, z);
			if (nCase < 255)
				break;

			z--;
		}

		if (bComputed)
			continue;

		AddNeighborsToList(nCase, x, y, z);

		while (m_nNumOpenVoxels)
		{
			m_nNumOpenVoxels--;
			x = m_pOpenVoxels[m_nNumOpenVoxels * 3];
			y = m_pOpenVoxels[m_nNumOpenVoxels * 3 + 1];
			z = m_pOpenVoxels[m_nNumOpenVoxels * 3 + 2];

			nCase = ComputeGridVoxel(x, y, z);

			AddNeighborsToList(nCase, x, y, z);
		}

	}

//	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(m_vertices, m_Triangles, m_UV0, m_normals, m_tangents);

	m_mesh->CreateMeshSection(1, m_vertices, m_Triangles, m_normals, m_UV0, m_vertexColors, m_tangents, false);


}



void AMetaballs::ComputeNormal(FVector vertex)
{

	float fSqDist;

	float n0 = 0.0f;
	float n1 = 0.0f;
	float n2 = 0.0f;

	for (int i = 0; i < m_NumBalls; i++)
	{
		float x = vertex.X - m_Balls[i].p.Z;
		float y = vertex.Y - m_Balls[i].p.Y;
		float z = vertex.Z - m_Balls[i].p.X;

		fSqDist = x*x + y*y + z*z;

		n0 = n0 + 2 * m_Balls[i].m * x / (fSqDist * fSqDist);
		n1 = n1 + 2 * m_Balls[i].m * y / (fSqDist * fSqDist);
		n2 = n2 + 2 * m_Balls[i].m * z / (fSqDist * fSqDist);

	}

	FVector normal = FVector(n0, n1, n2);
	normal.Normalize();
	
	m_normals.Add(normal);

	m_UV0.Add(FVector2D(normal.X, normal.Y));

}


void AMetaballs::AddNeighborsToList(int nCase, int x, int y, int z)
{
	if (CMarchingCubes::m_CubeNeighbors[nCase] & (1 << 0))
		AddNeighbor(x + 1, y, z);

	if (CMarchingCubes::m_CubeNeighbors[nCase] & (1 << 1))
		AddNeighbor(x - 1, y, z);

	if (CMarchingCubes::m_CubeNeighbors[nCase] & (1 << 2))
		AddNeighbor(x, y + 1, z);

	if (CMarchingCubes::m_CubeNeighbors[nCase] & (1 << 3))
		AddNeighbor(x, y - 1, z);

	if (CMarchingCubes::m_CubeNeighbors[nCase] & (1 << 4))
		AddNeighbor(x, y, z + 1);

	if (CMarchingCubes::m_CubeNeighbors[nCase] & (1 << 5))
		AddNeighbor(x, y, z - 1);
}


void AMetaballs::AddNeighbor(int x, int y, int z)
{
	if (IsGridVoxelComputed(x, y, z) || IsGridVoxelInList(x, y, z))
		return;

	// Make sure the array is large enough
	if (m_nMaxOpenVoxels == m_nNumOpenVoxels)
	{
		m_nMaxOpenVoxels *= 2;
		int *pTmp = new int[m_nMaxOpenVoxels * 3];
		memcpy(pTmp, m_pOpenVoxels, m_nNumOpenVoxels * 3 * sizeof(int));
		delete[] m_pOpenVoxels;
		m_pOpenVoxels = pTmp;
	}

	m_pOpenVoxels[m_nNumOpenVoxels * 3] = x;
	m_pOpenVoxels[m_nNumOpenVoxels * 3 + 1] = y;
	m_pOpenVoxels[m_nNumOpenVoxels * 3 + 2] = z;

	SetGridVoxelInList(x, y, z);

	m_nNumOpenVoxels++;
}

float AMetaballs::ComputeEnergy(float x, float y, float z)
{
	float fEnergy = 0;
	float fSqDist;

	for (int i = 0; i < m_NumBalls; i++)
	{
		// The formula for the energy is 
		// 
		//   e += mass/distance^2 

		fSqDist = (m_Balls[i].p.X - x)*(m_Balls[i].p.X - x) +
			(m_Balls[i].p.Y - y)*(m_Balls[i].p.Y - y) +
			(m_Balls[i].p.Z - z)*(m_Balls[i].p.Z - z);

		if (fSqDist < 0.0001f) fSqDist = 0.0001f;

		fEnergy += m_Balls[i].m / fSqDist;
	}

	return fEnergy;
}


float AMetaballs::ComputeGridPointEnergy(int x, int y, int z)
{
	if (IsGridPointComputed(x, y, z))
		return m_pfGridEnergy[x +
		y*(m_nGridSize + 1) +
		z*(m_nGridSize + 1)*(m_nGridSize + 1)];

	// The energy on the edges are always zero to make sure the isosurface is
	// always closed.
	if (x == 0 || y == 0 || z == 0 ||
		x == m_nGridSize || y == m_nGridSize || z == m_nGridSize)
	{
		m_pfGridEnergy[x +
			y*(m_nGridSize + 1) +
			z*(m_nGridSize + 1)*(m_nGridSize + 1)] = 0;
		SetGridPointComputed(x, y, z);
		return 0;
	}

	float fx = ConvertGridPointToWorldCoordinate(x);
	float fy = ConvertGridPointToWorldCoordinate(y);
	float fz = ConvertGridPointToWorldCoordinate(z);

	m_pfGridEnergy[x +
		y*(m_nGridSize + 1) +
		z*(m_nGridSize + 1)*(m_nGridSize + 1)] = ComputeEnergy(fx, fy, fz);

	SetGridPointComputed(x, y, z);

	return m_pfGridEnergy[x +
		y*(m_nGridSize + 1) +
		z*(m_nGridSize + 1)*(m_nGridSize + 1)];
}


int AMetaballs::ComputeGridVoxel(int x, int y, int z)
{
	float b[8];

	b[0] = ComputeGridPointEnergy(x, y, z);
	b[1] = ComputeGridPointEnergy(x + 1, y, z);
	b[2] = ComputeGridPointEnergy(x + 1, y, z + 1);
	b[3] = ComputeGridPointEnergy(x, y, z + 1);
	b[4] = ComputeGridPointEnergy(x, y + 1, z);
	b[5] = ComputeGridPointEnergy(x + 1, y + 1, z);
	b[6] = ComputeGridPointEnergy(x + 1, y + 1, z + 1);
	b[7] = ComputeGridPointEnergy(x, y + 1, z + 1);

	float fx = ConvertGridPointToWorldCoordinate(x) + m_fVoxelSize / 2;
	float fy = ConvertGridPointToWorldCoordinate(y) + m_fVoxelSize / 2;
	float fz = ConvertGridPointToWorldCoordinate(z) + m_fVoxelSize / 2;

	int c = 0;
	c |= b[0] > m_fLevel ? (1 << 0) : 0;
	c |= b[1] > m_fLevel ? (1 << 1) : 0;
	c |= b[2] > m_fLevel ? (1 << 2) : 0;
	c |= b[3] > m_fLevel ? (1 << 3) : 0;
	c |= b[4] > m_fLevel ? (1 << 4) : 0;
	c |= b[5] > m_fLevel ? (1 << 5) : 0;
	c |= b[6] > m_fLevel ? (1 << 6) : 0;
	c |= b[7] > m_fLevel ? (1 << 7) : 0;

	// Compute vertices from marching pyramid case
	fx = ConvertGridPointToWorldCoordinate(x);
	fy = ConvertGridPointToWorldCoordinate(y);
	fz = ConvertGridPointToWorldCoordinate(z);

	int i = 0;
	unsigned short EdgeIndices[12];
	memset(EdgeIndices, 0xFF, 12 * sizeof(unsigned short));

	float v0, v1, v2;
	while (1)
	{
		int nEdge = CMarchingCubes::m_CubeTriangles[c][i];
		if (nEdge == -1)
			break;

		if (EdgeIndices[nEdge] == 0xFFFF)
		{
			EdgeIndices[nEdge] = m_nNumVertices;

			// Compute the vertex by interpolating between the two points
			int nIndex0 = CMarchingCubes::m_CubeEdges[nEdge][0];
			int nIndex1 = CMarchingCubes::m_CubeEdges[nEdge][1];

			float t = (m_fLevel - b[nIndex0]) / (b[nIndex1] - b[nIndex0]);

			v0 = CMarchingCubes::m_CubeVertices[nIndex0][0] * (1 - t) + CMarchingCubes::m_CubeVertices[nIndex1][0] * t;
			v1 = CMarchingCubes::m_CubeVertices[nIndex0][1] * (1 - t) + CMarchingCubes::m_CubeVertices[nIndex1][1] * t;
			v2 = CMarchingCubes::m_CubeVertices[nIndex0][2] * (1 - t) + CMarchingCubes::m_CubeVertices[nIndex1][2] * t;

			v0 = fx + v0 * m_fVoxelSize;
			v1 = fy + v1 * m_fVoxelSize;
			v2 = fz + v2 * m_fVoxelSize;

			ComputeNormal(FVector(v2, v1, v0));

			m_vertices.Add(FVector(v2 * m_Scale, v1 * m_Scale, v0 * m_Scale));

			m_nNumVertices++;
		}

		m_Triangles.Add(EdgeIndices[nEdge]);

		m_nNumIndices++;

		i++;
	}

	SetGridVoxelComputed(x, y, z);

	return c;

}

float AMetaballs::ConvertGridPointToWorldCoordinate(int x)
{
	return float(x)*m_fVoxelSize - 1.0f;
}

int AMetaballs::ConvertWorldCoordinateToGridPoint(float x)
{
	return int((x + 1.0f) / m_fVoxelSize + 0.5f);
}

void AMetaballs::SetGridSize(int nSize)
{
	if (m_pfGridEnergy)
		delete m_pfGridEnergy;

	if (m_pnGridPointStatus)
		delete m_pnGridPointStatus;

	if (m_pnGridVoxelStatus)
		delete m_pnGridVoxelStatus;

	m_fVoxelSize = 2 / float(nSize);
	m_nGridSize = nSize;

	m_pfGridEnergy = new float[(nSize + 1)*(nSize + 1)*(nSize + 1)];
	m_pnGridPointStatus = new char[(nSize + 1)*(nSize + 1)*(nSize + 1)];
	m_pnGridVoxelStatus = new char[nSize*nSize*nSize];
}

inline bool AMetaballs::IsGridPointComputed(int x, int y, int z)
{
	if (m_pnGridPointStatus[x +
		y*(m_nGridSize + 1) +
		z*(m_nGridSize + 1)*(m_nGridSize + 1)] == 1)
		return true;
	else
		return false;
}

inline bool AMetaballs::IsGridVoxelComputed(int x, int y, int z)
{
	if (m_pnGridVoxelStatus[x +
		y*m_nGridSize +
		z*m_nGridSize*m_nGridSize] == 1)
		return true;
	else
		return false;
}

inline bool AMetaballs::IsGridVoxelInList(int x, int y, int z)
{
	if (m_pnGridVoxelStatus[x +
		y*m_nGridSize +
		z*m_nGridSize*m_nGridSize] == 2)
		return true;
	else
		return false;
}

inline void AMetaballs::SetGridPointComputed(int x, int y, int z)
{
	m_pnGridPointStatus[x +
		y*(m_nGridSize + 1) +
		z*(m_nGridSize + 1)*(m_nGridSize + 1)] = 1;
}

inline void AMetaballs::SetGridVoxelComputed(int x, int y, int z)
{
	m_pnGridVoxelStatus[x +
		y*m_nGridSize +
		z*m_nGridSize*m_nGridSize] = 1;
}

inline void AMetaballs::SetGridVoxelInList(int x, int y, int z)
{
	m_pnGridVoxelStatus[x +
		y*m_nGridSize +
		z*m_nGridSize*m_nGridSize] = 2;
}

void AMetaballs::InitBalls()
{
	FDateTime curTime;
	srand(curTime.GetTicks());

	// Ball 0 = core: always stays at origin in normalized space
	m_Balls[0].p    = FVector::ZeroVector;
	m_Balls[0].v    = FVector::ZeroVector;
	m_Balls[0].a    = FVector::ZeroVector;
	m_Balls[0].rest = FVector::ZeroVector;
	m_Balls[0].t    = 0.0f;
	m_Balls[0].m    = 1.0f;

	// Satellite balls: distribute rest positions evenly on a sphere surface
	// using the Fibonacci / golden-angle spiral (uniform distribution)
	const float SatelliteRadius = 0.3f;
	const float GoldenAngle     = PI * (3.0f - FMath::Sqrt(5.0f)); // ~2.399 rad
	const int   NumSatellites   = MAX_METABALLS - 1; // 31

	for (int i = 1; i < MAX_METABALLS; i++)
	{
		int   idx   = i - 1;
		float y     = (NumSatellites > 1)
		              ? 1.0f - (float(idx) / float(NumSatellites - 1)) * 2.0f
		              : 0.0f;
		float r     = FMath::Sqrt(FMath::Max(0.0f, 1.0f - y * y));
		float theta = GoldenAngle * float(idx);

		FVector dir(FMath::Cos(theta) * r, FMath::Sin(theta) * r, y);
		m_Balls[i].rest = dir * SatelliteRadius;
		m_Balls[i].p    = m_Balls[i].rest; // start at rest position
		m_Balls[i].v    = FVector::ZeroVector;
		m_Balls[i].t    = float(rand()) / RAND_MAX;
		m_Balls[i].m    = 1.0f;

		// Auto-mode target (used only when not player-controlled)
		m_Balls[i].a.X = m_AutoLimitY * (float(rand()) / RAND_MAX * 2 - 1);
		m_Balls[i].a.Y = m_AutoLimitX * (float(rand()) / RAND_MAX * 2 - 1);
		m_Balls[i].a.Z = m_AutoLimitZ * (float(rand()) / RAND_MAX * 2 - 1);

		// Optional small random offset from rest position
		if (m_randomseed)
		{
			m_Balls[i].p.X += (float(rand()) / RAND_MAX * 2 - 1) * 0.05f;
			m_Balls[i].p.Y += (float(rand()) / RAND_MAX * 2 - 1) * 0.05f;
			m_Balls[i].p.Z += (float(rand()) / RAND_MAX * 2 - 1) * 0.05f;
		}
	}
}


void AMetaballs::SetBallTransform(int32 index, FVector transfrom)
{
	if (index > m_NumBalls - 1)
	{
		return;
	}
	else
	{
		m_Balls[index].p.Y = transfrom.X;
		m_Balls[index].p.X = transfrom.Y;
		m_Balls[index].p.Z = transfrom.Z;
	}
}

void AMetaballs::SetNumBalls(int value)
{
	int32 ret = value;
	
	if (ret < 0)
	{
		ret = 0;
	}

	if (ret > AMetaballs::MAX_METABALLS)
	{
		ret = AMetaballs::MAX_METABALLS;
	}

	m_NumBalls = ret;
}


void AMetaballs::SetScale(float value)
{
	float ret = value;

	if (ret < AMetaballs::MIN_SCALE)
	{
		ret = AMetaballs::MIN_SCALE;
	}

	m_Scale = ret;

	// Keep collision capsule in sync with visual scale
	if (RootCapsule)
	{
		float capsuleSize = m_Scale * 0.8f;
		RootCapsule->SetCapsuleSize(capsuleSize, capsuleSize);
	}
}


void AMetaballs::SetGridSteps(int32 value)
{
	int32 ret = value;

	if (ret < AMetaballs::MIN_GRID_STEPS)
	{
		ret = AMetaballs::MIN_GRID_STEPS;
	}

	if (ret > AMetaballs::MAX_GRID_STEPS)
	{
		ret = AMetaballs::MAX_GRID_STEPS;
	}

	m_GridStep = ret;

	SetGridSize(m_GridStep);
}

void AMetaballs::SetRandomSeed(bool seed)
{
	m_randomseed = seed;
}

void AMetaballs::SetAutoMode(bool mode)
{
	m_automode = mode;
}

float AMetaballs::CheckLimit(float value)
{
	float ret = value;
	if (ret < AMetaballs::MIN_LIMIT)
	{
		ret = AMetaballs::MIN_LIMIT;
	}

	if (ret > AMetaballs::MAX_LIMIT)
	{
		ret = AMetaballs::MAX_LIMIT;
	}

	return ret;
}

void AMetaballs::SetAutoLimitX(float limit)
{
	m_AutoLimitX = CheckLimit(limit);
}

void AMetaballs::SetAutoLimitY(float limit)
{
	m_AutoLimitY = CheckLimit(limit);
}

void AMetaballs::SetAutoLimitZ(float limit)
{
	m_AutoLimitZ = CheckLimit(limit);
}