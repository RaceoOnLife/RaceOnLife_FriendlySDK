#include "Core/Library/RaceOnLifeLibrary.h"

/* ONLY WINDOWS INCLUDES */
#ifdef _WIN64
/* WINDOWS API */
#include <Windows.h>
#include "Windows/WindowsHWrapper.h"
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <combaseapi.h>
#include <endpointvolume.h>
#include <Microsoft/COMPointer.h>

/* AES CRYPTOGRAPHY */
//#include <aes.h>
#endif

/* FILES */
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/* PLATFORMS */
#include "HAL/PlatformFilemanager.h"

/* OTHER */
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include <Kismet/GameplayStatics.h>
#include "Camera/CameraComponent.h"

TArray<FString> URaceOnLifeLibrary::GetInputDevices()
{
	TArray<FString> InputDevices;

#ifdef _WIN64
	CoInitialize(NULL);

	IMMDeviceEnumerator* pEnum = NULL;
	IMMDeviceCollection* pDevices = NULL;

	CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnum));

	if (pEnum)
	{
		pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pDevices);
		if (pDevices)
		{
			UINT count;
			pDevices->GetCount(&count);
			for (UINT i = 0; i < count; ++i)
			{
				IMMDevice* pDevice = NULL;
				pDevices->Item(i, &pDevice);
				if (pDevice)
				{
					LPWSTR wstrID = NULL;
					pDevice->GetId(&wstrID);

					IPropertyStore* pStore;
					pDevice->OpenPropertyStore(STGM_READ, &pStore);
					if (pStore)
					{
						PROPVARIANT friendlyName;
						PropVariantInit(&friendlyName);
						pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);

						FString DeviceName(friendlyName.pwszVal);
						InputDevices.Add(DeviceName);

						PropVariantClear(&friendlyName);
						pStore->Release();
					}
					pDevice->Release();
				}
			}
			pDevices->Release();
		}
		pEnum->Release();
	}

	CoUninitialize();

	return InputDevices;
#endif
	return InputDevices;
}

TArray<FString> URaceOnLifeLibrary::GetOutputDevices()
{
	TArray<FString> OutputDevices;

#ifdef _WIN64
	CoInitialize(NULL);

	IMMDeviceEnumerator* pEnum = NULL;
	IMMDeviceCollection* pDevices = NULL;

	CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnum));

	if (pEnum)
	{
		pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
		if (pDevices)
		{
			UINT count;
			pDevices->GetCount(&count);
			for (UINT i = 0; i < count; ++i)
			{
				IMMDevice* pDevice = NULL;
				pDevices->Item(i, &pDevice);
				if (pDevice)
				{
					LPWSTR wstrID = NULL;
					pDevice->GetId(&wstrID);

					IPropertyStore* pStore;
					pDevice->OpenPropertyStore(STGM_READ, &pStore);
					if (pStore)
					{
						PROPVARIANT friendlyName;
						PropVariantInit(&friendlyName);
						pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);

						FString DeviceName(friendlyName.pwszVal);
						OutputDevices.Add(DeviceName);

						PropVariantClear(&friendlyName);
						pStore->Release();
					}
					pDevice->Release();
				}
			}
			pDevices->Release();
		}
		pEnum->Release();
	}

	CoUninitialize();
#endif

	return OutputDevices;
}

static HRESULT GetDeviceByName(const FString& DeviceName, IMMDevice** ppDevice, EDataFlow dataFlow)
{
	*ppDevice = nullptr;

	TComPtr<IMMDeviceEnumerator> pEnum = nullptr;
	TComPtr<IMMDeviceCollection> pDevices = nullptr;

	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnum));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = pEnum->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &pDevices);

	if (FAILED(hr))
	{
		return hr;
	}

	UINT count;
	pDevices->GetCount(&count);

	for (UINT i = 0; i < count; ++i)
	{
		TComPtr<IMMDevice> pDevice = nullptr;
		hr = pDevices->Item(i, &pDevice);

		if (SUCCEEDED(hr))
		{
			TComPtr<IPropertyStore> pStore = nullptr;
			hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);

			if (SUCCEEDED(hr))
			{
				PROPVARIANT friendlyName;
				PropVariantInit(&friendlyName);
				hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);

				if (SUCCEEDED(hr))
				{
					if (DeviceName.Equals(friendlyName.pwszVal, ESearchCase::IgnoreCase))
					{
						*ppDevice = pDevice.Get();
						pDevice.Detach();
						PropVariantClear(&friendlyName);
						return S_OK;
					}

					PropVariantClear(&friendlyName);
				}
			}
		}
	}

	return E_FAIL;
}

bool URaceOnLifeLibrary::SetInputDevice(const FString& DeviceName)
{
	CoInitialize(NULL);

	TComPtr<IMMDevice> pDevice = nullptr;
	HRESULT hr = GetDeviceByName(DeviceName, &pDevice, eCapture);

	if (FAILED(hr))
	{
		CoUninitialize();
		return false;
	}

	CoUninitialize();
	return false;
}

bool URaceOnLifeLibrary::SetOutputDevice(const FString& DeviceName)
{
	CoInitialize(NULL);

	TComPtr<IMMDevice> pDevice = nullptr;
	HRESULT hr = GetDeviceByName(DeviceName, &pDevice, eRender);

	if (FAILED(hr))
	{
		CoUninitialize();
		return false;
	}

	CoUninitialize();
	return false;
}

static HRESULT GetDefaultOutputDevice(IMMDevice** ppDevice)
{
	if (!ppDevice)
	{
		return E_POINTER;
	}

	*ppDevice = nullptr;

	TComPtr<IMMDeviceEnumerator> pEnum = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnum));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, ppDevice); // �������� ���������� �� ���������

	return hr;
}

static HRESULT GetDefaultInputDevice(IMMDevice** ppDevice)
{
	if (!ppDevice)
	{
		return E_POINTER;
	}

	*ppDevice = nullptr;

	TComPtr<IMMDeviceEnumerator> pEnum = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnum));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = pEnum->GetDefaultAudioEndpoint(eCapture, eConsole, ppDevice); // �������� ���������� �� ���������

	return hr;
}

static FString GetDeviceFriendlyName(IMMDevice* pDevice)
{
	if (!pDevice)
	{
		return FString("Unknown Device");
	}

	TComPtr<IPropertyStore> pStore = nullptr;
	HRESULT hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);

	if (FAILED(hr))
	{
		return FString("Unknown Device");
	}

	PROPVARIANT friendlyName;
	PropVariantInit(&friendlyName);
	hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);

	FString DeviceName = FString("Unknown Device");

	if (SUCCEEDED(hr))
	{
		DeviceName = friendlyName.pwszVal;
	}

	PropVariantClear(&friendlyName);

	return DeviceName;
}

FString URaceOnLifeLibrary::GetCurrentOutputDevice()
{
	IMMDevice* pOutputDevice = nullptr;
	FString OutputDeviceName = "Unknown Device";

	if (SUCCEEDED(GetDefaultOutputDevice(&pOutputDevice)))
	{
		OutputDeviceName = GetDeviceFriendlyName(pOutputDevice);
		pOutputDevice->Release();
	}

	return OutputDeviceName;
}

FString URaceOnLifeLibrary::GetCurrentInputDevice()
{
	IMMDevice* pInputDevice = nullptr;
	FString InputDeviceName = "Unknown Device";

	if (SUCCEEDED(GetDefaultInputDevice(&pInputDevice)))
	{
		InputDeviceName = GetDeviceFriendlyName(pInputDevice);
		pInputDevice->Release();
	}

	return InputDeviceName;
}

AActor* URaceOnLifeLibrary::GetClosestActorOfClass(TSubclassOf<AActor> ActorClass, APawn* PawnReference)
{
	if (!PawnReference || !ActorClass)
	{
		return nullptr;
	}

	AActor* ClosestActor = nullptr;
	float MinDistance = FLT_MAX;

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(PawnReference->GetWorld(), ActorClass, AllActors);

	for (AActor* Actor : AllActors)
	{
		float Distance = FVector::Dist(PawnReference->GetActorLocation(), Actor->GetActorLocation());

		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			ClosestActor = Actor;
		}
	}

	return ClosestActor;
}

FVector URaceOnLifeLibrary::CalculateImpulse(UCameraComponent* CameraComponent, float VehicleSpeed)
{
	if (!CameraComponent)
	{
		return FVector::ZeroVector; // Return zero vector if CameraComponent is null.
	}

	// Get the forward direction of the camera.
	FVector CameraForward = CameraComponent->GetForwardVector();

	FVector ImpulseDirection;
	float ImpulseStrength;

	if (VehicleSpeed <= 30.0f) // Speed is less than or equal to 30 km/h
	{
		// Weaker, lobbed impulse
		ImpulseStrength = 2250.0f; // Lower impulse strength for lower speeds.

		// Increase the vertical component for a lobbed trajectory.
		ImpulseDirection = CameraForward + FVector(0.0f, 0.0f, 1.0f);
	}
	else
	{
		// Normalize vehicle speed to ensure it's within a reasonable range.
		float NormalizedSpeed = FMath::Clamp(VehicleSpeed / 1000.0f, 0.0f, 1.0f);

		// Calculate impulse strength based on speed.
		ImpulseStrength = NormalizedSpeed * 4500.0f; // Adjust multiplier based on desired effect.

		// Create an arc-like trajectory by adjusting the vertical component of the direction.
		ImpulseDirection = CameraForward + FVector(0.0f, 0.0f, 0.5f); // Adding some vertical component.
	}

	ImpulseDirection.Normalize();

	// Calculate the final impulse vector.
	FVector Impulse = ImpulseDirection * ImpulseStrength;

	return Impulse;
}

/*

bool URaceOnLifeLibrary::IsItemBuyed(APlayerController* PlayerController, const FString& ItemID)
{
	TArray<FString> BuyedItems = GetBuyedItems(PlayerController);

	FString FullItemID = TEXT("shp_itm_") + ItemID;

	return BuyedItems.Contains(FullItemID);
}

FString URaceOnLifeLibrary::GetUserDataFilePath(APlayerController* PlayerController)
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Temp"), TEXT("userdata.json"));
}

TArray<FString> URaceOnLifeLibrary::GetBuyedItems(APlayerController* PlayerController)
{
	FString FilePath = GetUserDataFilePath(PlayerController);
	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		FString DecryptedContent = DecryptData(FileContent, TEXT("no_key_lol"));

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DecryptedContent);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			TArray<FString> Items;
			const TArray<TSharedPtr<FJsonValue>>* JsonArray;
			if (JsonObject->TryGetArrayField(TEXT("BuyedItems"), JsonArray))
			{
				for (auto& JsonValue : *JsonArray)
				{
					FString Item;
					if (JsonValue->TryGetString(Item))
					{
						Items.Add(Item);
					}
				}
			}
			return Items;
		}
	}
	return TArray<FString>();
}

void URaceOnLifeLibrary::SetBuyedItems(APlayerController* PlayerController, const TArray<FString>& Items)
{
	FString FilePath = GetUserDataFilePath(PlayerController);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> JsonArray;

	for (const FString& Item : Items)
	{
		JsonArray.Add(MakeShareable(new FJsonValueString(TEXT("shp_itm_") + Item)));
	}

	JsonObject->SetArrayField(TEXT("BuyedItems"), JsonArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	FString EncryptedContent = EncryptData(OutputString, TEXT("no_key_lol"));
	FFileHelper::SaveStringToFile(EncryptedContent, *FilePath);
}

void URaceOnLifeLibrary::AddBuyedItem(APlayerController* PlayerController, const FString& Item)
{
	TArray<FString> Items = GetBuyedItems(PlayerController);
	Items.Add(TEXT("shp_itm_") + Item);
	SetBuyedItems(PlayerController, Items);
}

float URaceOnLifeLibrary::GetBalance(APlayerController* PlayerController)
{
	FString FilePath = GetUserDataFilePath(PlayerController);
	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		FString DecryptedContent = DecryptData(FileContent, TEXT("no_key_lol"));

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DecryptedContent);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			return JsonObject->GetNumberField(TEXT("Balance"));
		}
	}
	return 0.0f;
}

void URaceOnLifeLibrary::SetBalance(APlayerController* PlayerController, float Balance)
{
	FString FilePath = GetUserDataFilePath(PlayerController);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetNumberField(TEXT("Balance"), Balance);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	FString EncryptedContent = EncryptData(OutputString, TEXT("no_key_lol"));
	FFileHelper::SaveStringToFile(EncryptedContent, *FilePath);
}

int32 URaceOnLifeLibrary::GetPlayedGames(APlayerController* PlayerController, const FString& GameMode)
{
	FString FilePath = GetUserDataFilePath(PlayerController);
	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		FString DecryptedContent = DecryptData(FileContent, TEXT("no_key_lol"));

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DecryptedContent);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			TSharedPtr<FJsonObject> PlayedGamesObject = JsonObject->GetObjectField(TEXT("PlayedGames"));
			return PlayedGamesObject->GetIntegerField(GameMode);
		}
	}
	return 0;
}

void URaceOnLifeLibrary::SetPlayedGames(APlayerController* PlayerController, const FString& GameMode, int32 Count)
{
	FString FilePath = GetUserDataFilePath(PlayerController);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedPtr<FJsonObject> PlayedGamesObject = MakeShareable(new FJsonObject());

	PlayedGamesObject->SetNumberField(GameMode, Count);
	JsonObject->SetObjectField(TEXT("PlayedGames"), PlayedGamesObject);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	FString EncryptedContent = EncryptData(OutputString, TEXT("no_key_lol"));
	FFileHelper::SaveStringToFile(EncryptedContent, *FilePath);
}

void URaceOnLifeLibrary::AddPlayedGame(APlayerController* PlayerController, const FString& GameMode)
{
	int32 Count = GetPlayedGames(PlayerController, GameMode);
	SetPlayedGames(PlayerController, GameMode, Count + 1);
}

FString URaceOnLifeLibrary::EncryptData(const FString& Data, const FString& Key)
{
	IBuffer plainBuffer = CryptographicBuffer::ConvertStringToBinary(Data, BinaryStringEncoding::Utf8);
	IBuffer keyBuffer = CryptographicBuffer::ConvertStringToBinary(Key, BinaryStringEncoding::Utf8);

	SymmetricKeyAlgorithmProvider aesProvider = SymmetricKeyAlgorithmProvider::OpenAlgorithm(L"AES_CBC_PKCS7");
	CryptographicKey aesKey = aesProvider.CreateSymmetricKey(keyBuffer);

	IBuffer ivBuffer = CryptographicBuffer::GenerateRandom(aesProvider.BlockLength());

	IBuffer encryptedBuffer = CryptographicEngine::Encrypt(aesKey, plainBuffer, ivBuffer);

	FString encryptedData = FString::Printf(TEXT("%s%s"),
		*CryptographicBuffer::EncodeToBase64String(ivBuffer),
		*CryptographicBuffer::EncodeToBase64String(encryptedBuffer));

	return encryptedData;
}

FString URaceOnLifeLibrary::DecryptData(const FString& EncryptedData, const FString& Key)
{
	FString ivBase64 = EncryptedData.Left(24);
	FString encryptedBase64 = EncryptedData.RightChop(24);

	IBuffer ivBuffer = CryptographicBuffer::DecodeFromBase64String(ivBase64);
	IBuffer encryptedBuffer = CryptographicBuffer::DecodeFromBase64String(encryptedBase64);
	IBuffer keyBuffer = CryptographicBuffer::ConvertStringToBinary(Key, BinaryStringEncoding::Utf8);

	SymmetricKeyAlgorithmProvider aesProvider = SymmetricKeyAlgorithmProvider::OpenAlgorithm(L"AES_CBC_PKCS7");
	CryptographicKey aesKey = aesProvider.CreateSymmetricKey(keyBuffer);

	IBuffer decryptedBuffer = CryptographicEngine::Decrypt(aesKey, encryptedBuffer, ivBuffer);

	FString decryptedData = CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, decryptedBuffer);

	return decryptedData;
}
*/

