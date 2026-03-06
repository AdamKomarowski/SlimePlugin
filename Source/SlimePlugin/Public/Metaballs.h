#pragma once
#include "SlimePluginPrivatePCH.h"
#include "GameFramework/Pawn.h"
#include "ProceduralMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "InputActionValue.h"
#include "Metaballs.generated.h"

class UInputMappingContext;
class UInputAction;


DECLARE_LOG_CATEGORY_EXTERN(YourLog, Log, All);

struct SMetaBall
{
	FVector p;     // position (normalized space [-1, 1])
	FVector v;     // velocity (normalized space)
	FVector a;     // acceleration / auto-mode target
	FVector rest;  // rest position for spring physics

	float t;       // time to next auto-mode direction change
	float m;       // mass
};


UCLASS()
class SLIMEPLUGIN_API AMetaballs : public APawn
{
	GENERATED_UCLASS_BODY()

public:

	enum MinMax
	{
		MAX_METABALLS   = 32,
		MIN_GRID_STEPS  = 16,
		MAX_GRID_STEPS  = 128,
		MIN_SCALE       = 1,
		MAX_OPEN_VOXELS = 32,
		MIN_LIMIT       = 0,
		MAX_LIMIT       = 1,
	};

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	                        bool bSelfMoved, FVector HitLocation, FVector HitNormal,
	                        FVector NormalImpulse, const FHitResult& Hit) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetBallTransform(int32 index, FVector transfrom);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetNumBalls(int32 value);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetScale(float value);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetGridSteps(int32 value);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetRandomSeed(bool seed);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetAutoMode(bool mode);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetAutoLimitX(float limit);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetAutoLimitY(float limit);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetAutoLimitZ(float limit);

	/*Number of metaballs (0 = disable)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Number of balls"))
	int32 m_NumBalls;

	/*Metaballs area scale*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Scale"))
	float m_Scale;

	/*Grid resolution — higher = more detail but slower*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Grid steps"))
	int32 m_GridStep;

	/*If true, balls start at random positions*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Random seed"))
	bool m_randomseed;

	/*If true and not possessed by player, balls animate automatically*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Auto fly mode"))
	bool m_automode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Auto limits X"))
	float m_AutoLimitX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Auto limit Y"))
	float m_AutoLimitY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Auto limit Z"))
	float m_AutoLimitZ;

	/*Metaballs material*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Material"))
	UMaterialInterface* m_Material;

	// --- Player physics ---

	/*Assign an InputMappingContext asset (create it in your project, map WASD to IA_SlimeMove)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Player", meta = (DisplayName = "Input Mapping Context"))
	UInputMappingContext* m_DefaultMappingContext;

	/*Assign an InputAction asset with Value Type = Axis2D (Vector2D)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Player", meta = (DisplayName = "Input Action Move"))
	UInputAction* m_InputMove;

	/*Force applied per input unit for player movement*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Player", meta = (DisplayName = "Move Speed"))
	float m_MoveSpeed;

	/*How stiffly satellite balls snap back to rest (higher = stiffer slime)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Spring Stiffness"))
	float m_SpringStiffness;

	/*Spring damping — reduces oscillation (higher = less bouncy)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Spring Damping"))
	float m_SpringDamping;

	/*How much satellite balls spread on collision impact*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Impact Spread Strength"))
	float m_ImpactSpreadStrength;

	/*How fast the spread decays back to normal after impact (higher = faster recovery)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Spread Decay Rate"))
	float m_SpreadDecayRate;

	// Components
	UPROPERTY(VisibleAnywhere, Category = "Slime")
	UCapsuleComponent* RootCapsule;

	UPROPERTY()
	UBoxComponent* MetaBallsBoundBox;

	UPROPERTY()
	UBillboardComponent* m_SpriteComponent;

	void  Update(float fDeltaTime);
	void  Render();
	void  SetGridSize(int nSize);

protected:

	void  InitBalls();
	float CheckLimit(float value);

	float ComputeEnergy(float x, float y, float z);
	void  ComputeNormal(FVector vertex);

	float ComputeGridPointEnergy(int x, int y, int z);
	int   ComputeGridVoxel(int x, int y, int z);

	bool  IsGridPointComputed(int x, int y, int z);
	bool  IsGridVoxelComputed(int x, int y, int z);
	bool  IsGridVoxelInList(int x, int y, int z);
	void  SetGridPointComputed(int x, int y, int z);
	void  SetGridVoxelComputed(int x, int y, int z);
	void  SetGridVoxelInList(int x, int y, int z);

	float ConvertGridPointToWorldCoordinate(int x);
	int   ConvertWorldCoordinateToGridPoint(float x);
	void  AddNeighborsToList(int nCase, int x, int y, int z);
	void  AddNeighbor(int x, int y, int z);

	// Enhanced Input handler (Axis2D: X = right, Y = forward)
	void Move(const FInputActionValue& Value);

	float m_fLevel;

	// Current spread multiplier: 1.0 = normal, >1.0 = spread after impact
	float m_SpreadAmount;

	SMetaBall m_Balls[AMetaballs::MAX_METABALLS];

	int   m_nNumOpenVoxels;
	int   m_nMaxOpenVoxels;
	int  *m_pOpenVoxels;

	int   m_nGridSize;
	float m_fVoxelSize;

	float *m_pfGridEnergy;
	char  *m_pnGridPointStatus;
	char  *m_pnGridVoxelStatus;

	int m_nNumVertices;
	int m_nNumIndices;

	UProceduralMeshComponent* m_mesh;

	TArray<FVector>          m_vertices;
	TArray<int32>            m_Triangles;
	TArray<FVector>          m_normals;
	TArray<FVector2D>        m_UV0;
	TArray<FColor>           m_vertexColors;
	TArray<FProcMeshTangent> m_tangents;
};
