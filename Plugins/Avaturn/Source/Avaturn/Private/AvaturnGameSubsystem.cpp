// Copyright © 2023++ Avaturn

#include "AvaturnGameSubsystem.h"

#include "AvaturnEditorActor.h"
#include "Utils/AvaturnRequestCreator.h"
#include "Utils/AvaturnGlTFConfigCreator.h"
#include "Json.h"
#include "Misc/Base64.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "GenericPlatform/GenericPlatformMisc.h"

#include "Utils/AvaturnUrlConvertor.h"
#include "Storage/AvaturnAvatarCacheHandler.h"

static const FString BaseHaircapsPath = "https://assets.avaturn.me/editor_resources/textures/haircaps/";
static const TMap<const FString, const FString> HaircapLinks =
{
	{"stripes", "stripes.jpg"},
	{"regular", "regular.jpg"},
	{"bald_jeoffrey", "bald_jeoffrey_fs.jpg"},
	{"bald_statham", "bald_statham_fs.jpg"},
	{"bald_soft", "bald_soft.jpg"},
	{"bald_zero", "bald_zero.jpg"}
};

UAvaturnGameSubsystem::UAvaturnGameSubsystem()
{}

void UAvaturnGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	ReadUInt16DataFromFile(CorrespIdsPath, CorrespIds);
}

void UAvaturnGameSubsystem::Deinitialize()
{
}

bool UAvaturnGameSubsystem::ReadUInt16DataFromFile(const FString& FilePath, TArray<uint16_t>& OutData)
{
	// Check if the file exists
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		UE_LOG(LogAvaturn, Warning, TEXT("File does not exist: %s"), *FilePath);
		return false;
	}

	// Create a file reader
	TUniquePtr<FArchive> FileReader(IFileManager::Get().CreateFileReader(*FilePath));

	if (!FileReader)
	{
		UE_LOG(LogAvaturn, Warning, TEXT("Failed to open file: %s"), *FilePath);
		return false;
	}

	// Read the file size
	int64 FileSize = FileReader->TotalSize();

	// Calculate the number of uint16_t integers
	int32 NumIntegers = FileSize / sizeof(uint16_t);

	OutData.SetNumUninitialized(NumIntegers);
	FileReader->Serialize(OutData.GetData(), FileSize);

	// Close the reader
	FileReader->Close();

	return true;
}

TSharedPtr<FJsonValue> UAvaturnGameSubsystem::ParseJSON(FString JsonResponse)
{
	TSharedPtr<FJsonValue> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(*JsonResponse);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		return JsonParsed;
	}

	return nullptr;
}

const FglTFRuntimeSkeletalMeshConfig& UAvaturnGameSubsystem::GetSkeletalMeshConfig()
{
	if (TargetSkeleton)
	{
		FAvaturnGlTFConfigCreator::OverrideConfig(SkeletalMeshConfig, "Armature", TargetSkeleton);
		SkeletalMeshConfig.SkeletonConfig.bNormalizeSkeletonScale = true;
		return SkeletalMeshConfig;
	}

	/*if (TargetSkeletonDataTable)
	{
		FString RowName = *FString::Printf(TEXT("%s_%d_%d"), *GetCurrentGenderString(), CurrentGenderTypeBody[1], GetCurrentBodyType());
		FTargetSkeleton* RowData = TargetSkeletonDataTable->FindRow<FTargetSkeleton>(*RowName, "", true);
		if (RowData->Skeleton)
		{
			FAvaturnGlTFConfigCreator::OverrideConfig(SkeletalMeshConfig, "Armature", RowData->Skeleton.LoadSynchronous());
		}
	}*/
	return SkeletalMeshConfig;
}

void UAvaturnGameSubsystem::SpawnEditorActor()
{
	if (!EditorActorClass)
	{
		EditorActorClass = AAvaturnEditorActor::StaticClass();
	}

	if (EditorActor)
	{
		return;
	}

	EditorActor = GetWorld()->SpawnActor<AAvaturnEditorActor>(EditorActorClass, FVector(0, 0, 50000), FRotator(0, 90, 0));

	TArray<FString> Keys;
	DefaultAssets.GetKeys(Keys);
	for (const FString& Key : Keys)
	{
		if (!EditorActor->DefaultAssets.Find(Key)->IsEmpty())
		{
			DefaultAssets.Emplace(Key, *EditorActor->DefaultAssets.Find(Key));
		}
	}

	BodyMeshComponent = EditorActor->BodyMeshComponent;
	HeadMeshComponent = EditorActor->HeadMeshComponent;
	LookMeshComponent = EditorActor->LookMeshComponent;
	EyesMeshComponent = EditorActor->EyesMeshComponent;
	ShoesMeshComponent = EditorActor->ShoesMeshComponent;
}

void UAvaturnGameSubsystem::InitAvaturnSDK(const FString& SessionLink)
{
	SpawnEditorActor();

	if (!SessionLink.IsEmpty())
	{
		// Parse Origin, Avatar Id, Custom Token
		FString NewLinkString = SessionLink;
		NewLinkString = NewLinkString.Replace(TEXT("https://"), TEXT(""));

		Origin = NewLinkString.Left(NewLinkString.Find(TEXT("/")));
		NewLinkString = NewLinkString.Replace(*(Origin + "/editor?customization_id="), TEXT(""));
		AvatarId = NewLinkString.Left(NewLinkString.Find(TEXT("&")));
		NewLinkString = NewLinkString.Replace(TEXT("&session_id="), TEXT(""));
		CustomToken = NewLinkString.Replace(*NewLinkString.Left(NewLinkString.Find(TEXT("=")) + 1), TEXT(""));
	}

	GetIdAndRefreshTokens();
}

void UAvaturnGameSubsystem::GetIdAndRefreshTokens()
{
	if (CustomToken.IsEmpty())
	{
		CustomTokenInvalidDelegate.ExecuteIfBound();
		return;
	}

	const FString EndPointUrl = FString::Printf(TEXT("https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken?key=%s"), *API_Key);
	const FString RequsetContent = FString::Printf(TEXT("{ \"token\": \"%s\", \"returnSecureToken\" : true }"), *CustomToken);
	AuthRequest = FAvaturnRequestCreator::MakeHttpRequest(EndPointUrl, 60.0f);
	AuthRequest->OnProcessRequestComplete().BindUObject(this, &UAvaturnGameSubsystem::OnTokensReceived);
	AuthRequest->SetContentAsString(RequsetContent);
	AuthRequest->SetVerb(TEXT("POST"));
	AuthRequest->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	AuthRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	AuthRequest->SetHeader(TEXT("Accepts"), TEXT("application/json"));
	AuthRequest->ProcessRequest();
}

void UAvaturnGameSubsystem::OnTokensReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bSuccess && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		TSharedPtr<FJsonObject> TokensJson = ParseJSON(Response->GetContentAsString())->AsObject();

		IdToken = TokensJson->GetStringField("idToken");
		RefreshToken = TokensJson->GetStringField("refreshToken");

		const FString AvatarUrl = FString::Printf(TEXT("https://api.avaturn.me/avatars/%s"), *AvatarId);
		const FString AuthorizationBearer = FString::Printf(TEXT("Bearer %s"), *IdToken);
		AuthRequest = FAvaturnRequestCreator::MakeHttpRequest(AvatarUrl, 60.0f);
		AuthRequest->OnProcessRequestComplete().BindUObject(this, &UAvaturnGameSubsystem::OnAvatarInfoReceived);
		AuthRequest->SetVerb("GET");
		AuthRequest->SetHeader("Origin", Origin);
		AuthRequest->SetHeader("User-Agent", "X-UnrealEngine-Agent");
		AuthRequest->SetHeader("Authorization", AuthorizationBearer);
		AuthRequest->ProcessRequest();
	}
	else
	{
		CustomTokenInvalidDelegate.ExecuteIfBound();

		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Yellow, TEXT("Get Tokens Request failed code: ") + FString::FromInt(Response->GetResponseCode())); }
	}
}

void UAvaturnGameSubsystem::OnAvatarInfoReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bSuccess && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		AvatarJson = ParseJSON(Response->GetContentAsString())->AsObject();

		FString Body = AvatarJson->GetObjectField("customization")->GetStringField("body");
		int32 BodyIndex = FCString::Atoi(*Body.Replace(TEXT("body_"), TEXT("")));

		FString Gender = AvatarJson->GetObjectField("scan")->GetObjectField("scan_metadata")->GetStringField("gender");
		int32 BodyGender = Gender == "female" ? 1 : 0;

		bool bAnimatable = false;
		int32 AnimatableIndex = AvatarJson->GetObjectField("scan")->GetObjectField("scan_metadata")->TryGetBoolField("animatable", bAnimatable) ? bAnimatable : false;

		EquipAvatarBody({ BodyGender, AnimatableIndex, BodyIndex });
	}
	else
	{
		CustomTokenInvalidDelegate.ExecuteIfBound();

		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Yellow, TEXT("Avatar Info Receive Failed: ") + FString::FromInt(Response->GetResponseCode()) + Response->GetContentAsString()); }
	}
}

void UAvaturnGameSubsystem::DownloadContent()
{
	ContentDownloadTime = FDateTime::Now();
	ContentRequest = FAvaturnRequestCreator::MakeHttpRequest("https://api.avaturn.me/content", 60.0f);
	ContentRequest->OnProcessRequestComplete().BindUObject(this, &UAvaturnGameSubsystem::OnContentDownloaded);
	ContentRequest->SetVerb("GET");
	ContentRequest->SetHeader("Origin", Origin);
	ContentRequest->ProcessRequest();
}

void UAvaturnGameSubsystem::OnContentDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bSuccess && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogAvaturn, Log, TEXT("Content Downloaded in [%.1fs]"), (FDateTime::Now() - ContentDownloadTime).GetTotalSeconds());

		LoadContent(ParseJSON(Response->GetContentAsString())->AsArray());
	}
}

void UAvaturnGameSubsystem::LoadContent(const TArray<TSharedPtr<FJsonValue>> ContentJson)
{
	TArray<FString> OldAssetTypes = { "look", "head", "eyes", "shoes" };

	for (TSharedPtr<FJsonValue> Element : ContentJson)
	{
		const int32 TypeIndex = OldAssetTypes.Find(Element->AsObject()->GetStringField("type"));
		if (TypeIndex == INDEX_NONE)
		{
			continue;
		}
		// asset types was renamed on server, but content still have old type names
		if (AssetTypes.IsValidIndex(TypeIndex))
		{
			FAvatarAsset Asset = FAvatarAsset();
			Asset.Id = Element->AsObject()->GetStringField("id");
			Asset.Category = AssetTypes[OldAssetTypes.Find(Element->AsObject()->GetStringField("type"))];
			Asset.Glb_Url = Element->AsObject()->GetStringField("glb_url").Replace(TEXT(".glb"), TEXT(".jpg.glb"));
			Asset.Preview_Url = Element->AsObject()->GetStringField("preview_url").Replace(TEXT(".webp"), TEXT(".png"));
			Asset.Gender = Element->AsObject()->GetStringField("gender");
			bool bDisableHead = false;
			Asset.bDisableHead = Element->AsObject()->GetObjectField("settings")->TryGetBoolField("with_head", bDisableHead) ? bDisableHead : false;
			bool bDisableShoes = false;
			Asset.bDisableShoes = Element->AsObject()->GetObjectField("settings")->TryGetBoolField("with_shoes", bDisableShoes) ? bDisableShoes : false;
			bool bOnlyHaircap = false;
			Asset.bOnlyHaircap = Element->AsObject()->GetObjectField("settings")->TryGetBoolField("only_haircap", bOnlyHaircap) ? bOnlyHaircap : false;
			FString HaircapId;
			Asset.HaircapId = Element->AsObject()->GetObjectField("settings")->TryGetStringField("haircap", HaircapId) ? HaircapId : DefaultHaircapId;
			bool bUseColorRamp = false;
			Asset.bUseColorRamp = Element->AsObject()->GetObjectField("settings")->TryGetBoolField("use_fresnel", bUseColorRamp) ? bUseColorRamp : true;
			Asset.Alias = Element->AsObject()->GetStringField("alias");

			AvatarAssets.Add(Asset);
		}
	}

	if (AvatarJson)
	{
		TArray<TSharedPtr<FJsonValue>> Keys;
		AvatarJson->GetObjectField("customization")->GetObjectField("assets")->Values.GenerateValueArray(Keys);

		TArray<FString> TempAssetTypes = AssetTypes;

		for (uint32 i = 0; i < 4; i++)
		{
			if (Keys.IsValidIndex(i))
			{
				FString ElementId;
				if (Keys[i]->AsObject()->TryGetStringField("id", ElementId))
				{
					AssetsToDownload.Add(ElementId);
					TempAssetTypes.Remove(GetAssetTypeById(ElementId));
					continue;
				}
			}

			TArray<FString> TypesToRemove;
			// set default assets if not set in server
			for (const FString& AssetType : TempAssetTypes)
			{
				const FString& AssetId = *DefaultAssets.Find(AssetType);
				if (!AssetId.IsEmpty())
				{
					AssetsToDownload.Add(AssetId);
					TypesToRemove.Add(GetAssetTypeById(AssetId));
				}
			}

			// remove types which replaced by default asset
			for (const FString& AssetType : TypesToRemove)
			{
				TempAssetTypes.Remove(AssetType);
			}

			// set first asset by type from avatar assets if not set by default asset
			for (const FString& AssetType : TempAssetTypes)
			{
				const FAvatarAsset* FoundData = AvatarAssets.FindByPredicate([&AssetType](const FAvatarAsset& Data) {return Data.Category == AssetType; });
				if (FoundData != nullptr)
				{
					AssetsToDownload.Add(FoundData->Id);
				}
			}
		}
	}

	if (AvatarJson->GetObjectField("customization")->GetObjectField("assets")->HasField("hair_and_headwear"))
	{
		SelectHairColor(AvatarJson->GetObjectField("customization")->GetObjectField("assets")->GetObjectField("hair_and_headwear")->GetStringField("color"));
	}
	else
	{
		// rework to get first color from list of colors
		SelectHairColor("#6d4539");
	}

	if (DownloadFromAssetsToDownload())
	{
	}
	else if (SepAssetsToDownload.IsValidIndex(0))
	{
		// download asset from query
		DownloadSeparateAsset();
	}

	AssetsLoadedDelegate.ExecuteIfBound();
}

const FString UAvaturnGameSubsystem::GetAssetTypeById(const FString& AssetId)
{
	const FAvatarAsset* FoundData = AvatarAssets.FindByPredicate([&AssetId](const FAvatarAsset& Data) {return Data.Id == AssetId; });
	if (FoundData != nullptr)
	{
		return FoundData->Category;
	}

	return FString();
}

const bool UAvaturnGameSubsystem::IsCurrentAssetsLoaded()
{
	if (AssetsRequest)
	{
		if (AssetsRequest->GetStatus() == EHttpRequestStatus::Processing)
		{
			return false;
		}
	}

	if (AssetsToDownload.IsValidIndex(0))
	{
		return false;
	}

	return true;
}

void UAvaturnGameSubsystem::AddSeparateAssetToQueue(const FString& Id, const FSeparateAssetLoaded& Response)
{
	const FAvatarAsset* FoundData = AvatarAssets.FindByPredicate([&Id](const FAvatarAsset& Data) {return Data.Id == Id; });
	if (FoundData != nullptr)
	{
		SepAssetsToDownload.Add(FSeparateAssetToDownload(Id, FoundData->Glb_Url, FoundData->Category, MakeShared<FSeparateAssetLoaded>(Response)));

		if (IsCurrentAssetsLoaded())
		{
			DownloadSeparateAsset();
		}
	}
}

void UAvaturnGameSubsystem::DownloadSeparateAsset()
{
	// CorrespIds need to retarget assets
	if (CorrespIds.Num() == 0)
	{
		return;
	}

	if (AssetsToDownload.Num() == 0 && bBodyInitialized)
	{
		const FSeparateAssetToDownload AssetToDownload = SepAssetsToDownload[0];
		SepAssetsToDownload.RemoveAt(0);
		SeparateAssetLoadedDelegate = *AssetToDownload.Delegate;
		DownloadAsset(AssetToDownload.Category, AssetToDownload.Url, AssetToDownload.Id, false);
	}
}

void UAvaturnGameSubsystem::DownloadAsset(const FString& AssetCategory, const FString& Url, const FString& Id, const bool bAutoSelect)
{
	// CorrespIds need to retarget assets
	if (CorrespIds.Num() == 0)
	{
		return;
	}

	if (AssetCategory != "body")
	{
		if (bAutoSelect)
		{
			const FAvatarAsset* CacheData = AvatarAssets.FindByPredicate([&Id](const FAvatarAsset& Data) {return Data.Id == Id; });
			int32 Index = 0;
			AssetTypes.Find(AssetCategory, Index);
			AvatarSelectedAssets[Index] = *CacheData;
		}
	}

	if (TryLoadFromCache(AssetCategory, Url, Id))
	{
		return;
	}

	ContentDownloadTime = FDateTime::Now();
	AssetsRequest = FAvaturnRequestCreator::MakeHttpRequest(Url, 60.0f);
	AssetsRequest->OnProcessRequestComplete().BindUObject(this, &UAvaturnGameSubsystem::OnAssetDownloaded);
	AssetsRequest->SetVerb("GET");
	AssetsRequest->SetHeader("Origin", Origin);
	AssetsRequest->SetHeader("AssetType", AssetCategory);
	AssetsRequest->SetHeader("Id", Id);
	AssetsRequest->ProcessRequest();
}

void UAvaturnGameSubsystem::OnAssetDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bSuccess && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogAvaturn, Log, TEXT("Asset Downloaded in [%.1fs] - %s"), (FDateTime::Now() - ContentDownloadTime).GetTotalSeconds(), *Request->GetHeader("AssetType"));

		LoadAsset(Request->GetHeader("AssetType"), Response->GetContent());


		CacheDownloadedFile(Request->GetURL(), Request->GetHeader("Id"), Response->GetContent());
	}
}

//// Cache Logic ////
void UAvaturnGameSubsystem::CacheDownloadedFile(const FString& Url, const FString& Id, const TArray<uint8>& Data, const FString& Extension)
{
	TOptional<FAssetUri> AvatarUri;
	TSharedPtr<class FAvaturnAvatarCacheHandler> CacheHandler;

	AvatarUri = FAvaturnUrlConvertor::CreateAssetUri(Url, Id, false, *Extension);

	CacheHandler = MakeShared<FAvaturnAvatarCacheHandler>(*AvatarUri);

	CacheHandler->SaveAssetInCache(AvatarUri->LocalModelPath, &Data);
}

const bool UAvaturnGameSubsystem::TryLoadFromCache(const FString& AssetCategory, const FString& Url, const FString& Id, const FString& Extension)
{
	//// CACHE LOGIC
	TOptional<FAssetUri> AvatarUri;
	TSharedPtr<class FAvaturnAvatarCacheHandler> CacheHandler;

	AvatarUri = FAvaturnUrlConvertor::CreateAssetUri(Url, Id, false, *Extension);

	CacheHandler = MakeShared<FAvaturnAvatarCacheHandler>(*AvatarUri);

	if (CacheHandler->ShouldLoadFromCache())
	{
		//We try to load from cache
		// Create a file reader
		TArray<uint8> Data;

		TUniquePtr<FArchive> FileReader(IFileManager::Get().CreateFileReader(*AvatarUri->LocalModelPath));

		if (FileReader)
		{
			// Read the file size and calc num of uint8
			int64 FileSize = FileReader->TotalSize();
			int32 NumIntegers = FileSize / sizeof(uint8);

			Data.SetNumUninitialized(NumIntegers);
			FileReader->Serialize(Data.GetData(), FileSize);
			FileReader->Close();

			if (Extension == ".glb")
			{
				LoadAsset(AssetCategory, Data);
			}
			else if(Extension == ".jpg")
			{
				LoadHaircapTexture(Data);
			}

			UE_LOG(LogAvaturn, Log, TEXT("Loaded Asset from disk: %s"), *Id);
			return true;
		}

		UE_LOG(LogAvaturn, Warning, TEXT("Failed to open asset: %s"), *AvatarUri->LocalModelPath);
	}
	return false;
}
//// Cache Logic ////

void UAvaturnGameSubsystem::LoadAsset(const FString AssetType, const TArray<uint8>& Data)
{
	int32 AssetIndex = 0;
	AssetTypes.Find(AssetType, AssetIndex);
	UglTFRuntimeAsset* Asset = nullptr;

	if (AssetType == "body")
	{
		UglTFRuntimeAsset* NewBodyAsset = UglTFRuntimeFunctionLibrary::glTFLoadAssetFromData(Data, FAvaturnGlTFConfigCreator::GetGlTFRuntimeConfig());

		if (BodyAsset == nullptr)
		{
			BodyAsset = NewBodyAsset;

			// {version}_{gender}_body_{i}
			const FString BodyCacheName = FString::Printf(TEXT("%s_%s_body_data_%d"), TEXT("v23_06_22"), *GetCurrentGenderString(), GetCurrentBodyType());
			DownloadAsset("body", BodyDataUrl, BodyCacheName);
			BodyDataUrl.Empty();
			return;
		}

		if (!BodyNormalsUrl.IsEmpty())
		{
			// and download normals
			// {version}_{gender}_body_{i}
			//const FString BodyCacheName = FString::Printf(TEXT("%s_%s_body_normal_%d"), TEXT("v23_06_22"), *GetCurrentGenderString(), GetCurrentBodyType());
			//DownloadAsset("body", BodyNormalsUrl, BodyCacheName);
			BodyNormalsUrl.Empty();
			//return;
		}

		/*TArray<TSharedPtr<FJsonValue>> Array = BodyAsset->GetParser()->GetJsonRoot()->GetArrayField("images");
		Array.Append(NewBodyAsset->GetParser()->GetJsonRoot()->GetArrayField("images"));
		BodyAsset->GetParser()->GetJsonRoot()->SetArrayField("images", Array);
		Array.Empty();
		Array = BodyAsset->GetParser()->GetJsonRoot()->GetArrayField("textures");
		Array.Append(NewBodyAsset->GetParser()->GetJsonRoot()->GetArrayField("textures"));
		BodyAsset->GetParser()->GetJsonRoot()->SetArrayField("textures", Array);*/

		//// Body Always must to have alphaMode - MASK
		const TArray<TSharedPtr<FJsonValue>>* JsonMaterials;
		if (!BodyAsset->GetParser()->GetJsonRoot()->TryGetArrayField("materials", JsonMaterials))
		{
			return;
		}

		for (int32 i = 0; i < (*JsonMaterials).Num(); i++)
		{
			TSharedPtr<FJsonObject> JsonMaterialObject = (*JsonMaterials)[i]->AsObject();
			if (!JsonMaterialObject)
			{
				return;
			}

			JsonMaterialObject->SetStringField("alphaMode", "MASK");
		}
		////

		// Mesh and Skeleton get from other assets and combine
		int32 SkinIndex;
		FglTFRuntimeMeshLOD AvatarLOD;
		FglTFRuntimeMeshLOD SkeletonLOD;
		BodyAsset->LoadMeshAsRuntimeLOD(0, AvatarLOD, GetSkeletalMeshConfig().MaterialsConfig);
		NewBodyAsset->LoadSkinnedMeshRecursiveAsRuntimeLOD(NewBodyAsset->GetNodes().Last().Name, {}, SkeletonLOD, GetSkeletalMeshConfig().MaterialsConfig, GetSkeletalMeshConfig().SkeletonConfig, SkinIndex);
		for (int32 i = 0; i < AvatarLOD.Primitives[0].Positions.Num(); i++)
		{
			if (SkeletonLOD.Primitives[0].Positions.IsValidIndex(i))
			{
				AvatarLOD.Primitives[0].Positions[i] = AvatarLOD.Primitives[0].Positions[i] + SkeletonLOD.Primitives[0].Positions[i];
			}
		}
		AvatarLOD.Primitives[0].Joints = SkeletonLOD.Primitives[0].Joints;
		AvatarLOD.Primitives[0].Weights = SkeletonLOD.Primitives[0].Weights;
		AvatarLOD.Primitives[0].OverrideBoneMap = SkeletonLOD.Primitives[0].OverrideBoneMap;

		AvatarVertexPositions = AvatarLOD.Primitives[0].Positions;
		BodyMeshComponent->SetSkeletalMesh(NewBodyAsset->LoadSkeletalMeshFromRuntimeLODs({ AvatarLOD }, 0, GetSkeletalMeshConfig()));
	}
	else
	{
		Asset = UglTFRuntimeFunctionLibrary::glTFLoadAssetFromData(Data, FAvaturnGlTFConfigCreator::GetGlTFRuntimeConfig());

		// Set Use Color Ramp for Hair
		if (AssetType == "hair_and_headwear")
		{
			const TArray<TSharedPtr<FJsonValue>>* JsonMaterials;
			if (!Asset->GetParser()->GetJsonRoot()->TryGetArrayField("materials", JsonMaterials))
			{
				return;
			}

			for (int32 i = 0; i < (*JsonMaterials).Num(); i++)
			{
				TSharedPtr<FJsonObject> JsonMaterialObject = (*JsonMaterials)[i]->AsObject();
				if (!JsonMaterialObject)
				{
					return;
				}


				JsonMaterialObject->SetBoolField("useColorRamp", AvatarSelectedAssets[AssetIndex].bUseColorRamp);
			}
		}

		// Remove junk prefix "mixamorig:"
		const TArray<TSharedPtr<FJsonValue>>* JsonNodes;
		if (!Asset->GetParser()->GetJsonRoot()->TryGetArrayField("nodes", JsonNodes))
		{
			return;
		}

		for (auto Element : (*JsonNodes))
		{
			FString BoneName = Element->AsObject()->GetStringField("name").Replace(TEXT("mixamorig:"), TEXT(""));
			Element->AsObject()->SetStringField("name", BoneName);
		}

		if (!SeparateAssetLoadedDelegate.IsBound())
		{
			AvatarRuntimeAssets[AssetIndex] = Asset;
		}
	}

	//// for separate download
	if (SeparateAssetLoadedDelegate.IsBound())
	{
		if (AssetType == "eyewear")
		{
			RetargetEyes(Asset, AssetIndex);
		}
		else
		{
			Retarget(Asset, AssetIndex);
		}

		SeparateAssetLoadedDelegate.Clear();

		// download asset from query
		if (SepAssetsToDownload.IsValidIndex(0))
		{
			DownloadSeparateAsset();
		}

		return;
	}
	////

	RetargetAssets(AssetType);

	// download asset from query
	if (DownloadFromAssetsToDownload())
	{
		return;
	}

	if (AssetsToDownload.Num() == 0 && bBodyInitialized)
	{
		// download asset from query
		if (SepAssetsToDownload.IsValidIndex(0))
		{
			DownloadSeparateAsset();
		}
	}
	////
}

void UAvaturnGameSubsystem::RetargetAssets(const FString& LastCategory)
{
	if (LastCategory == "body")
	{
		CopyBodyTexturesData();

		if (!bBodyInitialized)
		{
			DownloadContent();

			bBodyInitialized = true;
			return;
		}
	}

	// reaterget 4 assets: head, look, eyes, shoes
	for (uint8 i = 0; i < 4; i++)
	{
		if (AvatarRuntimeAssets[i])
		{
			if (AssetTypes[i] == "eyewear")
			{
				if (AvatarSelectedAssets[i].Alias.Contains("no_glasses"))
				{
					AvatarMeshAssets[i] = nullptr;
				}
				else
				{
					RetargetEyes(AvatarRuntimeAssets[i], i);
				}
			}
			else
			{
				if (AssetTypes[i] == "hair_and_headwear")
				{
					if (AvatarSelectedAssets[i].bOnlyHaircap)
					{
						AvatarMeshAssets[i] = nullptr;
						continue;
					}
				}

				Retarget(AvatarRuntimeAssets[i], i);
			}
		}
	}

	PrepareTextureData();

	UpdateAvatarMask();

	DownloadHairCap(AvatarSelectedAssets[AssetTypes.Find("hair_and_headwear")].HaircapId);

	AssignMeshes();

	ApplyRampColorToHair(nullptr);

	if (bAutoExport)
	{
		bAutoExport = false;
		MergedMeshLoadedDelegate.ExecuteIfBound();
		MergedMeshLoadedDelegate.Unbind();
	}

	// Send update to server only after all current assets loaded
	if (SepAssetsToDownload.Num() == 0 && IsCurrentAssetsLoaded())
	{
		SendUpdatedAvatarToServer();
	}
}

void UAvaturnGameSubsystem::SendUpdatedAvatarToServer()
{
	if (IdToken.IsEmpty() || RefreshToken.IsEmpty() || AvatarId.IsEmpty())
	{
		return;
	}

	FString NewAvatarDataString = GetAvatarData();
	if (AvatarDataString == NewAvatarDataString)
	{
		return;
	}

	if (AvatarDataString.IsEmpty())
	{
		AvatarDataString = NewAvatarDataString;
		return;
	}

	AvatarDataString = NewAvatarDataString;

	const FString AvatarUrl = FString::Printf(TEXT("https://api.avaturn.me/avatars/%s/customization"), *AvatarId);
	const FString AuthorizationBearer = FString::Printf(TEXT("Bearer %s"), *IdToken);
	AuthRequest = FAvaturnRequestCreator::MakeHttpRequest(AvatarUrl, 60.0f);
	AuthRequest->OnProcessRequestComplete().Unbind();
	AuthRequest->SetVerb("PUT");
	AuthRequest->SetHeader("Origin", Origin);
	AuthRequest->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	AuthRequest->SetHeader("Authorization", AuthorizationBearer);
	AuthRequest->SetContentAsString(AvatarDataString);
	AuthRequest->ProcessRequest();

	UE_LOG(LogAvaturn, Log, TEXT("%s"), *AvatarDataString);
}

const FString UAvaturnGameSubsystem::GetAvatarData()
{
	FString AvatarData;

	TSharedPtr<FJsonObject> AvatarDataJson = MakeShareable(new FJsonObject);
	AvatarDataJson->SetObjectField("metadata", MakeShareable(new FJsonObject));

	TSharedPtr<FJsonObject> AssetsJson = MakeShareable(new FJsonObject);
	for (uint32 i = 0; i < 4; i++)
	{
		if (AvatarSelectedAssets[i].Id.IsEmpty())
		{
			continue;
		}

		TSharedPtr<FJsonObject> AssetJson = MakeShareable(new FJsonObject);
		AssetJson->SetStringField("id", AvatarSelectedAssets[i].Id);

		if (AssetTypes[i] == "eyewear")
		{
			// for hair - hex code length = 8
			AssetJson->SetStringField("color", "#ffffff00");
		}

		if (AssetTypes[i] == "hair_and_headwear")
		{
			// for hair - hex code length = 6
			AssetJson->SetStringField("color", "#" + HairRampColor[1].ToFColor(true).ToHex().Left(6));
		}

		AssetsJson->SetObjectField(AssetTypes[i], AssetJson);
	}

	AvatarDataJson->SetObjectField("assets", AssetsJson);

	AvatarDataJson->SetStringField("body", FString::Printf(TEXT("body_%d"), GetCurrentBodyType()));
	AvatarDataJson->SetStringField("age", "adult");


	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&AvatarData);
	//Deserialize the json data given Reader and the actual object to deserialize
	if (FJsonSerializer::Serialize(AvatarDataJson.ToSharedRef(), Writer))
	{
		return AvatarData;
	}
	
	return FString();
}

void UAvaturnGameSubsystem::EquipAssetOnPlayer(const FString& Id, const FExportMeshLoaded& LoadedDelegate)
{
	MergedMeshLoadedDelegate = LoadedDelegate;
	bAutoExport = true;

	EquipAvatarAsset(Id);
}

void UAvaturnGameSubsystem::Retarget(UglTFRuntimeAsset* Asset, const int32 AssetIndex)
{
	if (!Asset)
	{
		return;
	}

	FglTFRuntimeMeshLOD LOD;
	int32 SkinIndex = -1;
	Asset->LoadSkinnedMeshRecursiveAsRuntimeLOD(Asset->GetNodes().Last().Name, {}, LOD, GetSkeletalMeshConfig().MaterialsConfig, GetSkeletalMeshConfig().SkeletonConfig, SkinIndex);

	TArray<FglTFRuntimePrimitive> Primitives = LOD.Primitives;

	for (int32 PrimitiveIndex = 0; PrimitiveIndex < Primitives.Num(); PrimitiveIndex++)
	{
		GetVertexExtraData(Asset, PrimitiveIndex);

		/* perfroms actual retargeting, `cloth_verts` is modified inplace  */
		const TArray<float> w = { 0.3137255, 0.24019608, 0.1764706, 0.12254902, 0.078431375, 0.04411765, 0.019607844, 0.004901961 };

		const uint64 num_verts = Primitives[PrimitiveIndex].Positions.Num();

		// Recompute verts inplace
		for (uint64 i = 0; i < num_verts; i++)
		{
			float sx = 0, sy = 0, sz = 0;

			for (uint8 k = 0; k < 8; k++)
			{
				uint64 v_id;

				if (k < 4)
				{
					v_id = CorrespIds[Ids0Array[i].Component(k)];
				}
				else
				{
					v_id = CorrespIds[Ids1Array[i].Component(k - 4)];
				}

				const FVector OldPos = AvatarVertexPositions[v_id];
				sx += w[k] * OldPos.X;
				sy += w[k] * OldPos.Y;
				sz += w[k] * OldPos.Z;
			}

			FVector pos = FVector::ZeroVector;
			pos.X = sx + VertexOffsets[i].X;
			pos.Y = sy + VertexOffsets[i].Y;
			pos.Z = sz + VertexOffsets[i].Z;

			Primitives[PrimitiveIndex].Positions[i] = pos;
		}

	}

	LOD.Primitives = Primitives;


	SkeletalMeshConfig.SkeletonConfig.CopyLocationsFrom = BodyMeshComponent->SkeletalMesh;
	if (SeparateAssetLoadedDelegate.IsBound())
	{
		USkeletalMesh* Mesh = Asset->LoadSkeletalMeshFromRuntimeLODs({ LOD }, SkinIndex, SkeletalMeshConfig);
		SeparateAssetLoadedDelegate.ExecuteIfBound(Mesh);
	}
	else
	{
		AvatarMeshAssets[AssetIndex] = Asset->LoadSkeletalMeshFromRuntimeLODs({ LOD }, SkinIndex, SkeletalMeshConfig);
	}
	SkeletalMeshConfig.SkeletonConfig.CopyLocationsFrom = nullptr;
}

void UAvaturnGameSubsystem::GetVertexExtraData(UglTFRuntimeAsset* Asset, const int32 PrimitiveIndex)
{
	if (!Asset->GetParser()->GetMeshes()[0]->GetArrayField("primitives").IsValidIndex(PrimitiveIndex))
	{
		return;
	}

	TSharedPtr<FJsonObject> JsonPrimitiveObject = Asset->GetParser()->GetMeshes()[0]->GetArrayField("primitives")[PrimitiveIndex]->AsObject();

	const TSharedPtr<FJsonObject>* JsonAttributesObject;
	if (!JsonPrimitiveObject->TryGetObjectField("attributes", JsonAttributesObject))
	{
		UE_LOG(LogAvaturn, Log, TEXT("LoadCloth(), No attributes array available"));
		return;
	}

	// POSITION is required for generating a valid Mesh
	if ((*JsonAttributesObject)->HasField("POSITION"))
	{
		FMatrix SceneBasis = FBasisVectorMatrix(FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0), FVector::ZeroVector);
		if (!Asset->GetParser()->BuildFromAccessorField(JsonAttributesObject->ToSharedRef(), "POSITION", VertexOffsets, { 3 }, { 5126 }, [&](FVector Value) -> FVector { return SceneBasis.TransformPosition(Value) * 100; }, INDEX_NONE, false, nullptr))
		{
			UE_LOG(LogAvaturn, Log, TEXT("LoadCloth(), Unable to load POSITION attribute"));
			return;
		}
	}
	else
	{
		UE_LOG(LogAvaturn, Log, TEXT("LoadCloth(), POSITION attribute is required"));
		return;
	}

	// _IDS_0 is required to get data from buffer
	if ((*JsonAttributesObject)->HasField("_IDS_0"))
	{
		if (!Asset->GetParser()->BuildFromAccessorField(JsonAttributesObject->ToSharedRef(), "_IDS_0", Ids0Array, { 4 }, { 5123 }, [&](FVector4 Value) -> FVector4 { return Value; }, INDEX_NONE, false, nullptr))
		{
			UE_LOG(LogAvaturn, Log, TEXT("LoadCloth(), Unable to load _IDS_0 attribute"));
			return;
		}
	}
	else
	{
		UE_LOG(LogAvaturn, Log, TEXT("LoadCloth(), _IDS_0 attribute is required"));
		return;
	}

	// _IDS_1 is required to get data from buffer
	if ((*JsonAttributesObject)->HasField("_IDS_1"))
	{
		if (!Asset->GetParser()->BuildFromAccessorField(JsonAttributesObject->ToSharedRef(), "_IDS_1", Ids1Array, { 4 }, { 5123 }, [&](FVector4 Value) -> FVector4 { return Value; }, INDEX_NONE, false, nullptr))
		{
			UE_LOG(LogAvaturn, Log, TEXT("LoadCloth(), Unable to load _IDS_1 attribute"));
			return;
		}
	}
	else
	{
		UE_LOG(LogAvaturn, Log, TEXT("LoadCloth(), _IDS_1 attribute is required"));
		return;
	}
}

void UAvaturnGameSubsystem::RetargetEyes(UglTFRuntimeAsset* Asset, const int32 AssetIndex)
{
	FglTFRuntimeMeshLOD LOD;
	int32 SkinIndex = -1;
	Asset->LoadSkinnedMeshRecursiveAsRuntimeLOD(Asset->GetNodes().Last().Name, {}, LOD, GetSkeletalMeshConfig().MaterialsConfig, GetSkeletalMeshConfig().SkeletonConfig, SkinIndex);

	TArray<FglTFRuntimePrimitive> Primitives = LOD.Primitives;

	for (int32 PrimitiveIndex = 0; PrimitiveIndex < Primitives.Num(); PrimitiveIndex++)
	{
		GetVertexExtraData(Asset, PrimitiveIndex);

		const TArray<FVector> InitialPositions = Primitives[PrimitiveIndex].Positions;

		const TSharedPtr<FJsonObject> GlassesAnnotation = Asset->GetParser()->GetJsonRoot()->GetArrayField("materials")[PrimitiveIndex]->AsObject()->GetObjectField("extras")->GetObjectField("glasses_annotation");
		const float TEMPLATE_HEAD_WIDTH = GlassesAnnotation->GetNumberField("head_width") * 100;
		const uint16 RIGHT_EAR_ID = CorrespIds[844];
		const uint16 LEFT_EAR_ID = CorrespIds[165];

		const float dx = AvatarVertexPositions[RIGHT_EAR_ID].X - AvatarVertexPositions[LEFT_EAR_ID].X;
		const float dy = AvatarVertexPositions[RIGHT_EAR_ID].Y - AvatarVertexPositions[LEFT_EAR_ID].Y;
		const float dz = AvatarVertexPositions[RIGHT_EAR_ID].Z - AvatarVertexPositions[LEFT_EAR_ID].Z;
		const double head_width = FMath::Sqrt(dx * dx + dy * dy + dz * dz);
		const double scale = head_width / TEMPLATE_HEAD_WIDTH;

		//
		const FVector nose_support_point = FVector(GlassesAnnotation->GetArrayField("nose_support_point")[0]->AsNumber(), GlassesAnnotation->GetArrayField("nose_support_point")[2]->AsNumber(), GlassesAnnotation->GetArrayField("nose_support_point")[1]->AsNumber()) * 100;
		const int32 nose_idx = GlassesAnnotation->GetNumberField("nose_idx");

		// Scale
		const FVector coordinates_scale = FVector(GlassesAnnotation->GetArrayField("coordinates_scale")[0]->AsNumber(), GlassesAnnotation->GetArrayField("coordinates_scale")[1]->AsNumber(), GlassesAnnotation->GetArrayField("coordinates_scale")[2]->AsNumber());
		const double scale_x = scale * coordinates_scale.X;
		const double scale_y = scale * coordinates_scale.Y;
		const double scale_z = scale * coordinates_scale.Z;

		const uint64 num_verts = Primitives[PrimitiveIndex].Positions.Num();

		// Retarget
		for (uint64 i = 0; i < num_verts; i++)
		{
			Primitives[PrimitiveIndex].Positions[i].X = InitialPositions[i].X * scale_x;
			Primitives[PrimitiveIndex].Positions[i].Y = InitialPositions[i].Y * scale_y;
			Primitives[PrimitiveIndex].Positions[i].Z = InitialPositions[i].Z * scale_z;
		}

		const double shift_x = AvatarVertexPositions[nose_idx].X - nose_support_point.X * scale;
		const double shift_y = AvatarVertexPositions[nose_idx].Y - nose_support_point.Y * scale;
		const double shift_z = AvatarVertexPositions[nose_idx].Z - nose_support_point.Z * scale;

		for (uint64 i = 0; i < num_verts; i++)
		{
			Primitives[PrimitiveIndex].Positions[i].X += shift_x;
			Primitives[PrimitiveIndex].Positions[i].Y += shift_y;
			Primitives[PrimitiveIndex].Positions[i].Z += shift_z;
		}
	}

	LOD.Primitives = Primitives;


	SkeletalMeshConfig.SkeletonConfig.CopyLocationsFrom = BodyMeshComponent->SkeletalMesh;
	if (SeparateAssetLoadedDelegate.IsBound())
	{
		USkeletalMesh* Mesh = Asset->LoadSkeletalMeshFromRuntimeLODs({ LOD }, SkinIndex, SkeletalMeshConfig);
		SeparateAssetLoadedDelegate.ExecuteIfBound(Mesh);
	}
	else
	{
		AvatarMeshAssets[AssetIndex] = Asset->LoadSkeletalMeshFromRuntimeLODs({ LOD }, SkinIndex, SkeletalMeshConfig);
	}
	SkeletalMeshConfig.SkeletonConfig.CopyLocationsFrom = nullptr;
}

void UAvaturnGameSubsystem::PrepareTextureData()
{
	TArray<FglTFRuntimeMipMap> Mips;
	FglTFRuntimeMaterialsConfig MaterialsConfig = FglTFRuntimeMaterialsConfig();
	MaterialsConfig.ImagesConfig.MaxHeight = 2048;
	MaterialsConfig.ImagesConfig.MaxWidth = 2048;
	FglTFRuntimeTextureSampler Sampler;

	AssetTextures.Empty();

	uint8 Index = -1;
	for (UglTFRuntimeAsset* RuntimeAsset : AvatarRuntimeAssets)
	{
		Index++;
		// temp: fix bad alpha map from head and eyes
		if (Index == AssetTypes.Find("hair_and_headwear") || Index == AssetTypes.Find("eyewear"))
		{
			continue;
		}

		if (RuntimeAsset)
		{
			const TArray<TSharedPtr<FJsonValue>>* JsonImages;
			if (RuntimeAsset->GetParser()->GetJsonRoot()->TryGetArrayField("images", JsonImages))
			{
				for (uint8 i = 0; i < JsonImages->Num(); i++)
				{
					if (JsonImages->GetData()[i].ToSharedRef()->AsObject()->GetStringField("name") == "occlusion")
					{
						AssetTextures.Add(RuntimeAsset->GetParser()->LoadTexture(i, Mips, false, MaterialsConfig, Sampler));
						break;
					}
				}
			}
		}
	}
}

void UAvaturnGameSubsystem::UpdateAvatarMask()
{
	if (!AlphaTexture)
	{
		AlphaTexture = UTexture2D::CreateTransient(512, 512);
		AlphaTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
		AlphaTexture->SRGB = false;
#if WITH_EDITOR
		AlphaTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif
	}

	uint8* RawImageData = static_cast<uint8*>(AlphaTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	const uint32 TextureSize = AlphaTexture->GetSizeX() * AlphaTexture->GetSizeY();

	// Set alpha to one first
	for (uint32 i = 0; i < TextureSize; i++)
	{
		// change Alpha channel value
		RawImageData[i * 4] = 0;
		RawImageData[i * 4 + 1] = 0;
		RawImageData[i * 4 + 2] = 255;
		RawImageData[i * 4 + 3] = 255;
	}

	// Add mask from each cloth
	for (UTexture2D* AssetTexture : AssetTextures)
	{
		if (AssetTexture)
		{
			const uint8* ClothRawData = static_cast<uint8*>(AssetTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY));

			for (uint32 i = 0; i < TextureSize; i++)
			{
				// this is basically OR operation
				RawImageData[i * 4 + 2] = ((ClothRawData[i * 4] & 1) > 0) * RawImageData[i * 4 + 2];
			}

			AssetTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
		}
	}

	AlphaTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
	AlphaTexture->UpdateResource();

	Cast<UMaterialInstanceDynamic>(BodyMeshComponent->GetMaterials()[0])->SetTextureParameterValue("alphaTexture", AlphaTexture);
}

void UAvaturnGameSubsystem::ApplyRampColorToHair(USkeletalMeshComponent* HeadMesh)
{
	USkeletalMeshComponent* SkelMeshToModify = nullptr;

	if (HeadMesh)
	{
		SkelMeshToModify = HeadMesh;
	}
	else if (AvatarSelectedAssets[AssetTypes.Find("hair_and_headwear")].bUseColorRamp)
	{
		SkelMeshToModify = HeadMeshComponent;
	}

	if (!SkelMeshToModify)
	{
		return;
	} 

	if (SkelMeshToModify->GetMaterials().Num() > 0)
	{
		for (UMaterialInterface* Material : SkelMeshToModify->GetMaterials())
		{
			Cast<UMaterialInstanceDynamic>(Material)->SetVectorParameterValue("ramp_color1", HairRampColor[0]);
			Cast<UMaterialInstanceDynamic>(Material)->SetVectorParameterValue("ramp_color2", HairRampColor[1]);
			Cast<UMaterialInstanceDynamic>(Material)->SetVectorParameterValue("ramp_color3", HairRampColor[2]);
		}
	}
}

void UAvaturnGameSubsystem::DownloadHairCap(const FString& HaircapType)
{
	const FString LocalHaircapType = HaircapType.IsEmpty() ? DefaultHaircapId : HaircapType;
	HaircapTexture = nullptr;
	if (TryLoadFromCache("", "", LocalHaircapType, ".jpg"))
	{
		return;
	}
	ContentDownloadTime = FDateTime::Now();
	HaircapRequest = FAvaturnRequestCreator::MakeHttpRequest(GetHaircapLinkByType(LocalHaircapType), 60.0f);
	HaircapRequest->OnProcessRequestComplete().BindUObject(this, &UAvaturnGameSubsystem::OnHaircapDownloaded);
	HaircapRequest->SetVerb("GET");
	HaircapRequest->SetHeader("Origin", Origin);
	HaircapRequest->SetHeader("Id", LocalHaircapType);
	HaircapRequest->ProcessRequest();
}

const FString UAvaturnGameSubsystem::GetHaircapLinkByType(const FString& HaircapType)
{
	return (BaseHaircapsPath + *HaircapLinks.Find(HaircapType));
}

void UAvaturnGameSubsystem::OnHaircapDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bSuccess && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogAvaturn, Log, TEXT("Haircap Downloaded in [%.1fs]"), (FDateTime::Now() - ContentDownloadTime).GetTotalSeconds());

		LoadHaircapTexture(Response->GetContent());

		CacheDownloadedFile(Request->GetURL(), Request->GetHeader("Id"), Response->GetContent(), ".jpg");
	}
}

void UAvaturnGameSubsystem::LoadHaircapTexture(const TArray<uint8>& Data)
{
	HaircapTexture = UKismetRenderingLibrary::ImportBufferAsTexture2D(this, Data);

	EnableHairCap();
}

void UAvaturnGameSubsystem::EnableHairCap()
{
	if (!HaircapTexture || !BodyMeshComponent->GetMaterial(0))
	{
		return;
	}

	UTexture* Texture = nullptr;
	FHashedMaterialParameterInfo Params = FHashedMaterialParameterInfo();
	Params.Name = FMemoryImageMaterialParameterInfo(TEXT("baseColorTexture")).Name;
	BodyMeshComponent->GetMaterial(0)->GetTextureParameterValue(Params, Texture);
	UTexture2D* BaseColorTexture = Cast<UTexture2D>(Texture);

	//Params.Name = FMemoryImageMaterialParameterInfo(TEXT("metallicRoughnessTexture")).Name;
	//BodyMeshComponent->GetMaterial(0)->GetTextureParameterValue(Params, Texture);
	//UTexture2D* RoughnessTexture = Cast<UTexture2D>(Texture);

	TextureCompressionSettings BaseColorCompressionSettings = BaseColorTexture->CompressionSettings;
	//TextureCompressionSettings RoughnessCompressionSettings = RoughnessTexture->CompressionSettings;
	BaseColorTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	//RoughnessTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;

#if WITH_EDITOR
	TextureMipGenSettings BaseColorMipGenSettings = BaseColorTexture->MipGenSettings;
	//TextureMipGenSettings RoughnessMipGenSettings = RoughnessTexture->MipGenSettings;
	BaseColorTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	//RoughnessTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif

	bool BaseColorSRGB = BaseColorTexture->SRGB;
	//bool RoughnessSRGB = RoughnessTexture->SRGB;
	BaseColorTexture->SRGB = false;
	//RoughnessTexture->SRGB = false;

	BaseColorTexture->UpdateResource();
	//RoughnessTexture->UpdateResource();

	uint8* RawBaseColorData = (uint8*)BaseColorTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	//uint8* RawRoughnessData = (uint8*)RoughnessTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	HaircapTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	HaircapTexture->SRGB = false;
	HaircapTexture->UpdateResource();

	uint8* RawHaircapData = (uint8*)HaircapTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY);

	const uint32 TextureSize = BaseColorTexture->GetSizeY() * BaseColorTexture->GetSizeX();
	//const uint32 RoughnessTextureSize = RoughnessTexture->GetSizeY() * RoughnessTexture->GetSizeX();

	for (uint32 i = 0; i < TextureSize; i++)
	{
		const float X = BaseColorTexturePixels[i * 4 + 2];
		const float Y = BaseColorTexturePixels[i * 4 + 1];
		const float Z = BaseColorTexturePixels[i * 4];
		const float valR = RawHaircapData[i * 4 + 2] / 255.0f;
		const float valG = RawHaircapData[i * 4 + 1] / 255.0f;
		const FVector HairCap = ApplyHairCap(FVector(X, Y, Z), valG);
		RawBaseColorData[i * 4 + 2] = HairCap.X;
		RawBaseColorData[i * 4 + 1] = HairCap.Y;
		RawBaseColorData[i * 4] = HairCap.Z;

		//if (i < RoughnessTextureSize)
		//{
		//	// Rougness Map in Green slot of ORM
		//	const uint8 Roughness = RawRoughnessData[i * 4 + 1];
		//	RawRoughnessData[i * 4 + 1] = ApplyHairCapRoughness(Roughness, valR, valG);
		//}
	}

	BaseColorTexture->CompressionSettings = BaseColorCompressionSettings;
	//RoughnessTexture->CompressionSettings = RoughnessCompressionSettings;

#if WITH_EDITOR
	BaseColorTexture->MipGenSettings = BaseColorMipGenSettings;
	//RoughnessTexture->MipGenSettings = RoughnessMipGenSettings;
#endif

	BaseColorTexture->SRGB = BaseColorSRGB;
	//RoughnessTexture->SRGB = RoughnessSRGB;

	BaseColorTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
	//RoughnessTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
	HaircapTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

	BaseColorTexture->UpdateResource();
	//RoughnessTexture->UpdateResource();
}

void UAvaturnGameSubsystem::AssignMeshes()
{
	bool bDisableHead = false;
	bool bDisableShoes = false;
	TArray<USkeletalMeshComponent*> SkeletalComponents = { LookMeshComponent, HeadMeshComponent, EyesMeshComponent, ShoesMeshComponent };
	for (const FAvatarAsset Asset : AvatarSelectedAssets)
	{
		if (Asset.bDisableHead == true)
		{
			bDisableHead = true;
		}

		if (Asset.bDisableShoes == true)
		{
			bDisableShoes = true;
		}
	}
	for (uint8 i = 0; i < 4; i++)
	{
		if (AvatarMeshAssets[i])
		{
			if (i == AssetTypes.Find("hair_and_headwear") && bDisableHead)
			{
				SkeletalComponents[i]->SetSkeletalMesh(nullptr);
			}
			else
			{
				if (i == AssetTypes.Find("footwear") && bDisableShoes)
				{
					SkeletalComponents[i]->SetSkeletalMesh(nullptr);
				}
				else
				{
					SkeletalComponents[i]->SetSkeletalMesh(AvatarMeshAssets[i]);

					// set 0.5f specular for all clothes
					Cast<UMaterialInstanceDynamic>(AvatarMeshAssets[i]->GetMaterials()[0].MaterialInterface.Get())->SetScalarParameterValue("specularFactor", 0.5f);
				}
			}
		}
		else
		{
			SkeletalComponents[i]->SetSkeletalMesh(nullptr);
		}
	}
}

const bool UAvaturnGameSubsystem::DownloadFromAssetsToDownload()
{
	if (AssetsToDownload.IsValidIndex(0))
	{
		const FString AssetToDownload = AssetsToDownload[0];
		AssetsToDownload.RemoveAt(0);
		EquipAvatarAsset(AssetToDownload);
		return true;
	}

	return false;
}

void UAvaturnGameSubsystem::EquipAvatarAsset(const FString& Id)
{
	const FAvatarAsset* FoundData = AvatarAssets.FindByPredicate([&Id](const FAvatarAsset& Data) {return Data.Id == Id; });
	if (FoundData != nullptr)
	{
		DownloadAsset(FoundData->Category, FoundData->Glb_Url, FoundData->Id);
	}
	else
	{
		if (DownloadFromAssetsToDownload())
		{
		}
		else if (SepAssetsToDownload.IsValidIndex(0))
		{
			// download asset from query
			DownloadSeparateAsset();
		}
	}
}

void UAvaturnGameSubsystem::EquipAvatarBody(const TArray<int32> NewGenderTypeBody)
{
	bool bNeedDownloadAsset = false;
	int32 LastBodyType = CurrentGenderTypeBody[2];

	for (uint8 i = 0; i < NewGenderTypeBody.Num(); i++)
	{
		if (NewGenderTypeBody.IsValidIndex(i))
		{
			if (NewGenderTypeBody[i] != -1)
			{
				bNeedDownloadAsset = CurrentGenderTypeBody[i] != NewGenderTypeBody[i] ? true : bNeedDownloadAsset;

				CurrentGenderTypeBody[i] = NewGenderTypeBody[i];
			}
		}
	}

	if (bNeedDownloadAsset || !BodyAsset)
	{
		// to do: there we will make url to download gender and face anims avatar depend on CurrentGenderTypeBody
		BodyMeshUrl = AvatarJson->GetStringField("scan_glb_url");
		BodyDataUrl = FString::Printf(TEXT("https://assets.avaturn.me/editor_resources/bodies/v23_06_22/%s/adult_%d_nocompression.glb"), *GetCurrentGenderString(), GetCurrentBodyType());
		BodyNormalsUrl = FString::Printf(TEXT("https://assets.avaturn.me/editor_resources/bodies/materials/normals/%s_%d.jpg"), *GetCurrentGenderString(), GetCurrentBodyType());
		BodyAsset = nullptr;

		// {version}_{gender}_body_{i}
		const FString BodyCacheName= FString::Printf(TEXT("%s_%s_body"), TEXT("v23_06_22"), TEXT("scan"));
		DownloadAsset("body", BodyMeshUrl, BodyCacheName);
		BodyMeshUrl.Empty();
	}
	else
	{
		if (LastBodyType == CurrentGenderTypeBody[2])
		{
			return;
		}

		// when we change body, need to retarget all assets
		RetargetAssets("body");
	}
}

/** Converts a linear space RGB color to an HSL color */
FLinearColor LinearRGBToHSL(const FLinearColor& Color)
{
	const float RGBMin = FMath::Min3(Color.R, Color.G, Color.B);
	const float RGBMax = FMath::Max3(Color.R, Color.G, Color.B);
	const float RGBRange = RGBMax - RGBMin;

	float Hue = (RGBRange == 0.0f ? 0.0f :
		RGBMax == Color.R ? ((Color.G - Color.B) / RGBRange + (Color.G < Color.B ? 6.0f : 0)) :
		RGBMax == Color.G ? ((Color.B - Color.R) / RGBRange + 2.0f) :
		/*RGBMax == Color.B*/(Color.R - Color.G) / RGBRange + 4.0f);

	Hue /= 6;

	const float Lightness = (RGBMax + RGBMin) / 2.0f;
	const float Saturation = (RGBRange == 0.0f ? 0.0f : 
		Lightness <= 0.5f ? RGBRange / (RGBMax + RGBMin) : RGBRange / (2.0f - RGBMax - RGBMin));

	// In the new color, R = H, G = S, B = L, A = A
	return FLinearColor(Hue, Saturation, Lightness, Color.A);
}

const float HueToRGB(float p, float q, float t) 
{

	if (t < 0.0f) t += 1;
	if (t > 1.0f) t -= 1;
	if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
	if (t < 1.0f / 2.0f) return q;
	if (t < 2.0f / 3.0f) return p + (q - p) * 6.0f * (2.0f / 3.0f - t);
	return p;

}

/** Converts an HSL color to a linear space RGB color */
FLinearColor HSLToLinearRGB(const FLinearColor& Color)
{
	// In this color, R = H, G = S, B = L
	const float Hue = Color.R;
	const float Saturation = Color.G;
	const float Lightness = Color.B;

	if (Saturation == 0.0f)
	{
		return FLinearColor(Lightness, Lightness, Lightness);
	}
	else
	{
		const float p = Lightness <= 0.5f ? Lightness * (1.0f + Saturation) : Lightness + Saturation - (Lightness * Saturation);
		const float q = (2.0f * Lightness) - p;

		const float R = HueToRGB(q, p, Hue + 1.0f / 3.0f);
		const float G = HueToRGB(q, p, Hue);
		const float B = HueToRGB(q, p, Hue - 1.0f / 3.0f);
		return FLinearColor(R, G, B, Color.A);
	}
}

FVector UAvaturnGameSubsystem::ApplyHairCap(const FVector DiffuseColor, const float map_val)
{
	FLinearColor HSL = HairRampColor[1];
	HSL = LinearRGBToHSL(HSL);

	HSL.R = FMath::Fmod((HSL.R + 0.5f + 0.5f), 1.0f);
	HSL.G = FMath::Clamp(HSL.G * 1.2f, 0.0f, 1.0f);
	HSL.B = HSL.B * 0.3f;
	FColor NewColor = HSLToLinearRGB(HSL).ToFColor(true);

	float hair_cap_multiplier = 1.0f;
	const FVector hair_cap_color = FVector(NewColor.R, NewColor.G, NewColor.B);

	float fac = FMath::Clamp(((1.0f - map_val) * hair_cap_multiplier), 0.0f, 1.0f);
	return FMath::Lerp(hair_cap_color, DiffuseColor, fac);
}

uint8 UAvaturnGameSubsystem::ApplyHairCapRoughness(const uint8 roughness, const float map_valR, const float map_valG)
{
	float fac_bald = FMath::Clamp(map_valR, 0, 255);
	float fac_hair = FMath::Clamp(map_valG, 0, 255);

	return FMath::Lerp(FMath::Lerp(0.25f * 255, roughness, fac_bald), 0.759f, fac_hair);
}

void UAvaturnGameSubsystem::SelectHairColor(const FString& HairColor)
{
	if (HairRampColor[1].ToFColor(true).ToHex() == HairColor)
	{
		return;
	}

	const float l3 = 1.95f, s3 = 0.58f, h3 = 0.95f, l1 = 0.03f, s1 = 1.32f;

	FColor NewColor = FColor::FromHex(HairColor);
	FLinearColor HSL = FLinearColor(NewColor);
	HSL = LinearRGBToHSL(HSL);

	FLinearColor ColorB = FLinearColor(0, 0, 0);

	ColorB.R = FMath::Fmod((HSL.R + 0.49f + 0.5f), 1.0f);
	ColorB.G = FMath::Clamp(HSL.G * s1, 0.0f, 1.0f);
	ColorB.B = HSL.B * l1;
	ColorB = HSLToLinearRGB(ColorB);
	HairRampColor[0] = ColorB;

	HairRampColor[1] = FLinearColor(NewColor);

	ColorB.R = FMath::Fmod((HSL.R + 0.51f + 0.5f), 1.0f);
	ColorB.G = HSL.G * s3;
	ColorB.B = FMath::Clamp(HSL.B * l3, 0.0f, 0.6f);
	ColorB = HSLToLinearRGB(ColorB);
	HairRampColor[2] = ColorB;

	ApplyRampColorToHair(nullptr);

	EnableHairCap();

	SendUpdatedAvatarToServer();
}

const FColor UAvaturnGameSubsystem::GetHairColorByIndex(const int32 ColorIndex)
{
	return FColor::FromHex(HairColors[ColorIndex]);
}

uint8 UAvaturnGameSubsystem::GetCurrentBodyType()
{
	return CurrentGenderTypeBody[2];
}

uint8 UAvaturnGameSubsystem::GetCurrentGenderType()
{
	return CurrentGenderTypeBody[0];
}

const FString UAvaturnGameSubsystem::GetCurrentGenderString()
{
	return GetCurrentGenderType() ? "female" : "male";
}

const FString UAvaturnGameSubsystem::GetSelectedAssetIdByCategory(const FString& Category)
{
	if (AvatarSelectedAssets.IsValidIndex(AssetTypes.Find(Category)))
	{
		return AvatarSelectedAssets[AssetTypes.Find(Category)].Id;
	}
	return FString();
}

void UAvaturnGameSubsystem::CopyBodyTexturesData()
{
	if (!BodyMeshComponent->GetMaterial(0))
	{
		return;
	}

	UTexture* Texture = nullptr;
	FHashedMaterialParameterInfo Params = FHashedMaterialParameterInfo();
	Params.Name = FMemoryImageMaterialParameterInfo(TEXT("baseColorTexture")).Name;
	BodyMeshComponent->GetMaterial(0)->GetTextureParameterValue(Params, Texture);
	UTexture2D* BaseColorTexture = Cast<UTexture2D>(Texture);

	//Params.Name = FMemoryImageMaterialParameterInfo(TEXT("metallicRoughnessTexture")).Name;
	//BodyMeshComponent->GetMaterial(0)->GetTextureParameterValue(Params, Texture);
	//UTexture2D* RoughnessTexture = Cast<UTexture2D>(Texture);

	TextureCompressionSettings BaseColorCompressionSettings = BaseColorTexture->CompressionSettings;
	//TextureCompressionSettings RoughnessCompressionSettings = RoughnessTexture->CompressionSettings;
	BaseColorTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	//RoughnessTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;

#if WITH_EDITOR
	TextureMipGenSettings BaseColorMipGenSettings = BaseColorTexture->MipGenSettings;
	//TextureMipGenSettings RoughnessMipGenSettings = RoughnessTexture->MipGenSettings;
	BaseColorTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	//RoughnessTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif

	bool BaseColorSRGB = BaseColorTexture->SRGB;
	//bool RoughnessSRGB = RoughnessTexture->SRGB;
	BaseColorTexture->SRGB = false;
	//RoughnessTexture->SRGB = false;

	uint8* RawBaseColorData = static_cast<uint8*>(BaseColorTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
	//uint8* RawRoughnessData = (uint8*)RoughnessTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY);

	int32 TextureSize = BaseColorTexture->GetSizeX() * BaseColorTexture->GetSizeY() * 4;
	BaseColorTexturePixels.Empty();
	BaseColorTexturePixels.AddDefaulted(TextureSize);
	FMemory::Memcpy(BaseColorTexturePixels.GetData(), RawBaseColorData, TextureSize);

	//TextureSize = RoughnessTexture->GetSizeX() * RoughnessTexture->GetSizeY() * 4;
	//RoughnessTexturePixels.Empty();
	//RoughnessTexturePixels.AddDefaulted(TextureSize);
	//FMemory::Memcpy(RoughnessTexturePixels.GetData(), RawRoughnessData, TextureSize);

	BaseColorTexture->CompressionSettings = BaseColorCompressionSettings;
	//RoughnessTexture->CompressionSettings = RoughnessCompressionSettings;

#if WITH_EDITOR
	BaseColorTexture->MipGenSettings = BaseColorMipGenSettings;
	//RoughnessTexture->MipGenSettings = RoughnessMipGenSettings;
#endif

	BaseColorTexture->SRGB = BaseColorSRGB;
	//RoughnessTexture->SRGB = RoughnessSRGB;

	BaseColorTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
	//RoughnessTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
}