#pragma once
#include "SlimePluginPrivatePCH.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Metaballs.generated.h"


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
class SLIMEPLUGIN_API AMetaballs : public AActor
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

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

	/** Called from your Character Blueprint every tick to move the slime to the character's location. */
	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetSlimePosition(FVector WorldPosition);

	/**
	 * Call this from your Character Blueprint when the character lands (e.g. on OnLanded event).
	 * Triggers a squish-and-recover impulse: balls spread outward then spring back.
	 * @param Strength  How hard the impact is (0.5 = light, 2.0 = heavy landing).
	 */
	UFUNCTION(BlueprintCallable, Category = "Slime")
	void TriggerImpact(float Strength);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetGravity(float value);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetBounciness(float value);

	UFUNCTION(BlueprintCallable, Category = "Slime")
	void SetDamping(float value);

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

	/*If true, balls animate automatically (decorative mode). If false, use SetSlimePosition() to drive the slime from a character.*/
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

	/*
	 * Dungeon Slime technique: force applied to each ball when moving.
	 * The force falls off with squared distance from center-of-mass,
	 * so balls near the core respond first, outer balls lag behind → slorpy undulating movement.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Input Force Strength"))
	float m_InputForceStrength;

	/*How fast input force drops off with distance from center of mass (higher = force stays closer to core)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Input Falloff Rate"))
	float m_InputFalloffRate;

	/*Gravity acceleration pulling balls downward (world Z axis)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Gravity"))
	float m_Gravity;

	/*Bounciness on wall/floor collision (0=no bounce, 1=perfect elastic)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Bounciness", ClampMin = "0.0", ClampMax = "1.0"))
	float m_Bounciness;

	/*Velocity damping simulating slime viscosity (0=no damping, 1=instant stop)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Physics", meta = (DisplayName = "Damping", ClampMin = "0.0", ClampMax = "1.0"))
	float m_Damping;

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

	float m_fLevel;

	// Current spread multiplier: 1.0 = normal, >1.0 = spread after impact
	float m_SpreadAmount;

	// Movement delta in normalized ball-space, set by SetSlimePosition each frame
	FVector m_MoveDelta;

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
