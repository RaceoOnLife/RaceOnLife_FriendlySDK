// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdVisualizationCustomDataProcessor.h"
#include "MassCrowdRepresentationSubsystem.h"
#include "MassRepresentationFragments.h"
#include "MassVisualizationComponent.h"
#include "CrowdVisualizationFragment.h"
#include "MassCrowdAnimationTypes.h"
#include "MassUpdateISMProcessor.h"
#include "MassCrowdUpdateISMVertexAnimationProcessor.h"
#include "GameFramework/PlayerController.h"
#include "MassLODFragments.h"

namespace UE::CitySampleCrowd
{
	int32 bAllowKeepISMExtraFrameBetweenISM = 1;
	FAutoConsoleVariableRef CVarAllowKeepISMExtraFrameBetweenISM(TEXT("CitySample.crowd.AllowKeepISMExtraFrameBetweenISM"), bAllowKeepISMExtraFrameBetweenISM, TEXT("Allow the frost crowd visulaization to keep previous ISM an extra frame when switching between ISM"), ECVF_Default);

	int32 bAllowKeepISMExtraFrameWhenSwitchingToActor = 0;
	FAutoConsoleVariableRef CVarAllowKeepISMExtraFrameWhenSwitchingToActor(TEXT("CitySample.crowd.AllowKeepISMExtraFrameWhenSwitchingToActor"), bAllowKeepISMExtraFrameWhenSwitchingToActor, TEXT("Allow the frost crowd visulaization to keep ISM an extra frame when switching to spanwed actor"), ECVF_Default);

}

UMassProcessor_CrowdVisualizationCustomData::UMassProcessor_CrowdVisualizationCustomData()
	: EntityQuery_Conditional(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);

	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Representation);

	// Requires animation system to update vertex animation data first
	ExecutionOrder.ExecuteAfter.Add(TEXT("MassProcessor_Animation"));

	bRequiresGameThreadExecution = true; // due to read-write access to FMassRepresentationSubsystemSharedFragment
}

void UMassProcessor_CrowdVisualizationCustomData::ConfigureQueries()
{
	EntityQuery_Conditional.AddRequirement<FCrowdAnimationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery_Conditional.AddRequirement<FCitySampleCrowdVisualizationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery_Conditional.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery_Conditional.AddRequirement<FMassViewerInfoFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery_Conditional.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery_Conditional.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery_Conditional.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery_Conditional.SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk);
	EntityQuery_Conditional.AddSharedRequirement<FMassRepresentationSubsystemSharedFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassProcessor_CrowdVisualizationCustomData::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery_Conditional.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			UMassProcessor_CrowdVisualizationCustomData::UpdateCrowdCustomData(Context);
		});
}

void UMassProcessor_CrowdVisualizationCustomData::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
	World = Owner.GetWorld();
	check(World);
	LODSubsystem = UWorld::GetSubsystem<UMassLODSubsystem>(World);
}

void UMassProcessor_CrowdVisualizationCustomData::UpdateCrowdCustomData(FMassExecutionContext& Context)
{
	UMassRepresentationSubsystem* RepresentationSubsystem = Context.GetMutableSharedFragment<FMassRepresentationSubsystemSharedFragment>().RepresentationSubsystem;
	check(RepresentationSubsystem);
	FMassInstancedStaticMeshInfoArrayView ISMInfos = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();

	const int32 NumEntities = Context.GetNumEntities();
	TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
	TArrayView<FMassRepresentationFragment> RepresentationList = Context.GetMutableFragmentView<FMassRepresentationFragment>();
	TConstArrayView<FMassViewerInfoFragment> LODInfoList = Context.GetFragmentView<FMassViewerInfoFragment>();
	TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();
	TConstArrayView<FCitySampleCrowdVisualizationFragment> CitySampleCrowdVisualizationList = Context.GetFragmentView<FCitySampleCrowdVisualizationFragment>();
	TArrayView<FCrowdAnimationFragment> AnimationDataList = Context.GetMutableFragmentView<FCrowdAnimationFragment>();

	// 0-4 are anim data
	// 5-7 are color varations on clothing
	// 5 is an atlas index on skin
	const int NumCustomFloatsPerISM = 8;
	const int ColorVariationIndex = 5;
	const int AtlasVariationIndex = 5;

	const int HeadIdx = 0;
	const int BodyIdx = 1;
	const int TopIdx = 2;
	const int BottomIdx = 3;
	const int ShoesIdx = 4;

	TArray<float, TInlineAllocator<3>> CustomFloats;
	CustomFloats.AddZeroed(3);
	float& R = CustomFloats[0];
	float& G = CustomFloats[1];
	float& B = CustomFloats[2];

	

	for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
	{
		const FTransformFragment& TransformFragment = TransformList[EntityIdx];
		FMassRepresentationFragment& Representation = RepresentationList[EntityIdx];
		const FMassViewerInfoFragment& LODInfo = LODInfoList[EntityIdx];
		const FMassRepresentationLODFragment& RepresentationLOD = RepresentationLODList[EntityIdx];
		const FCitySampleCrowdVisualizationFragment& CitySampleCrowdVisualization = CitySampleCrowdVisualizationList[EntityIdx];
		FCrowdAnimationFragment& AnimationData = AnimationDataList[EntityIdx];

		if (Representation.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance || 
			// Keeping an extra frame of ISM when switching to actors as sometime the actor isn't loaded and will not be display on lower hand platform.
		    ( UE::CitySampleCrowd::bAllowKeepISMExtraFrameWhenSwitchingToActor && 
			  Representation.PrevRepresentation == EMassRepresentationType::StaticMeshInstance && 
			 (Representation.CurrentRepresentation == EMassRepresentationType::LowResSpawnedActor || Representation.CurrentRepresentation == EMassRepresentationType::HighResSpawnedActor)) )
		{
			// @todo find a better way to do this
			// Skip instances unseen by ExclusiveInstanceViewerIdx 
			//if (ExclusiveInstanceViewerIdx == INDEX_NONE || LODInfo.bIsVisibleByViewer[ExclusiveInstanceViewerIdx])
			{
				FMassInstancedStaticMeshInfo& ISMInfo = ISMInfos[Representation.StaticMeshDescIndex];

				const float PrevLODSignificance = UE::CitySampleCrowd::bAllowKeepISMExtraFrameBetweenISM ? Representation.PrevLODSignificance : -1.0f;

				// Update Transform
				UMassUpdateISMProcessor::UpdateISMTransform(GetTypeHash(Context.GetEntity(EntityIdx)), ISMInfo, TransformFragment.GetTransform(), Representation.PrevTransform, RepresentationLOD.LODSignificance, PrevLODSignificance);

				// Custom data layout is 0-4 are anim data, 5-7 are color variations, 5 is an atlas index on some meshes
				// Need 3 floats of padding after anim data
				const int32 CustomDataPaddingAmount = 3;

				// Add Vertex animation custom floats
				UMassCrowdUpdateISMVertexAnimationProcessor::UpdateISMVertexAnimation(ISMInfo, AnimationData, RepresentationLOD.LODSignificance, PrevLODSignificance, CustomDataPaddingAmount);

				// Add color custom floats
				R = (CitySampleCrowdVisualization.TopColor >> 24) / 255.0f;
				G = ((CitySampleCrowdVisualization.TopColor >> 16) & 0xff) / 255.0f;
				B = ((CitySampleCrowdVisualization.TopColor >> 8) & 0xff) / 255.0f;
				ISMInfo.WriteCustomDataFloatsAtStartIndex(TopIdx, CustomFloats, RepresentationLOD.LODSignificance, NumCustomFloatsPerISM, ColorVariationIndex, PrevLODSignificance);
				R = (CitySampleCrowdVisualization.BottomColor >> 24) / 255.0f;
				G = ((CitySampleCrowdVisualization.BottomColor >> 16) & 0xff) / 255.0f;
				B = ((CitySampleCrowdVisualization.BottomColor >> 8) & 0xff) / 255.0f;
				ISMInfo.WriteCustomDataFloatsAtStartIndex(BottomIdx, CustomFloats, RepresentationLOD.LODSignificance, NumCustomFloatsPerISM, ColorVariationIndex, PrevLODSignificance);
				R = (CitySampleCrowdVisualization.ShoesColor >> 24) / 255.0f;
				G = ((CitySampleCrowdVisualization.ShoesColor >> 16) & 0xff) / 255.0f;
				B = ((CitySampleCrowdVisualization.ShoesColor >> 8) & 0xff) / 255.0f;
				ISMInfo.WriteCustomDataFloatsAtStartIndex(ShoesIdx, CustomFloats, RepresentationLOD.LODSignificance, NumCustomFloatsPerISM, ColorVariationIndex, PrevLODSignificance);

				// Add skin atlas custom floats
				TArray<float, TInlineAllocator<1>> SkinAtlasIndex({ float(CitySampleCrowdVisualization.SkinAtlasIndex) });
				ISMInfo.WriteCustomDataFloatsAtStartIndex(HeadIdx, SkinAtlasIndex, RepresentationLOD.LODSignificance, NumCustomFloatsPerISM, AtlasVariationIndex, PrevLODSignificance);
				ISMInfo.WriteCustomDataFloatsAtStartIndex(BodyIdx, SkinAtlasIndex, RepresentationLOD.LODSignificance, NumCustomFloatsPerISM, AtlasVariationIndex, PrevLODSignificance);
			}
		}
		Representation.PrevTransform = TransformFragment.GetTransform();
		Representation.PrevLODSignificance = RepresentationLOD.LODSignificance;
	}
}