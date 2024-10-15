// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "VoxelGPUBufferReadback.h"
#include "TextureResource.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"

// Needed to cancel motion blur when reusing proxies
#include "ScenePrivate.h"

void RHIUpdateTexture2D_Safe(
	FRHITexture* Texture,
	const uint32 MipIndex,
	const FUpdateTextureRegion2D& UpdateRegion,
	const uint32 SourcePitch,
	const TConstVoxelArrayView<uint8> SourceData)
{
	check(Texture);
	VOXEL_SCOPE_COUNTER_FORMAT("RHIUpdateTexture2D_Safe %s", *Texture->GetName().ToString());

	// FD3D12Texture::UpdateTexture2D doesn't use SrcX/SrcX
	check(UpdateRegion.SrcX == 0);
	check(UpdateRegion.SrcY == 0);

	// See FD3D11DynamicRHI::RHIUpdateTexture2D
	const FPixelFormatInfo& FormatInfo = GPixelFormats[Texture->GetFormat()];
	const size_t UpdateHeightInTiles = FMath::DivideAndRoundUp(UpdateRegion.Height, uint32(FormatInfo.BlockSizeY));
	const size_t SourceDataSize = static_cast<size_t>(SourcePitch) * UpdateHeightInTiles;
	check(SourceData.Num() >= SourceDataSize);

	if (!ensure(UpdateRegion.DestX + UpdateRegion.Width <= Texture->GetSizeX()) ||
		!ensure(UpdateRegion.DestY + UpdateRegion.Height <= Texture->GetSizeY()))
	{
		return;
	}

	RHIUpdateTexture2D_Unsafe(
		Texture,
		MipIndex,
		UpdateRegion,
		SourcePitch,
		SourceData.GetData());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FMaterialRenderProxy* FVoxelUtilities::CreateColoredMaterialRenderProxy(
	FMeshElementCollector& Collector,
	const FLinearColor& Color,
	const UMaterialInterface* Material)
{
	if (!Material)
	{
		Material = GEngine->ShadedLevelColorationUnlitMaterial;
	}
	if (!ensure(Material))
	{
		return nullptr;
	}

	FColoredMaterialRenderProxy* MaterialProxy = new FColoredMaterialRenderProxy(Material->GetRenderProxy(), Color);
	Collector.RegisterOneFrameMaterialProxy(MaterialProxy);
	return MaterialProxy;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelUtilities::UpdateTextureRef(
	UTexture2D* TextureObject,
	FRHITexture* TextureRHI)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInRenderingThread());

	if (!ensure(TextureObject) ||
		!ensure(TextureRHI))
	{
		return false;
	}

	ensure(TextureObject->GetSizeX() == TextureRHI->GetSizeX());
	ensure(TextureObject->GetSizeY() == TextureRHI->GetSizeY());
	ensure(TextureObject->GetPixelFormat() == TextureRHI->GetFormat());

	FTextureResource* Resource = TextureObject->GetResource();
	if (!ensure(Resource))
	{
		return false;
	}

	FTextureRHIRef OldTextureRHI = Resource->TextureRHI;
	Resource->TextureRHI = TextureRHI;

	struct FHack : FTextureResource
	{
		static FTextureReferenceRHIRef& GetReference(FTextureResource* Resource)
		{
			return static_cast<FHack*>(Resource)->TextureReferenceRHI;
		}
	};

	const FTextureReferenceRHIRef& TextureReferenceRHI = FHack::GetReference(Resource);
	if (ensure(TextureReferenceRHI.IsValid()))
	{
		VOXEL_SCOPE_COUNTER("RHIUpdateTextureReference");
		RHIUpdateTextureReference(TextureReferenceRHI, Resource->TextureRHI);
	}

	VOXEL_SCOPE_COUNTER("SafeRelease");
	OldTextureRHI.SafeRelease();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFuture FVoxelUtilities::AsyncCopyTexture(
	const TWeakObjectPtr<UTexture2D> TargetTexture,
	const TSharedRef<const TVoxelArray<uint8>>& Data)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	if (!ensure(TargetTexture.IsValid()))
	{
		return FVoxelFuture::Done();
	}

	const int32 SizeX = TargetTexture->GetSizeX();
	const int32 SizeY = TargetTexture->GetSizeY();
	const EPixelFormat Format = TargetTexture->GetPixelFormat();
	if (!ensure(Data->Num() == GPixelFormats[Format].BlockBytes * SizeX * SizeY))
	{
		return FVoxelFuture::Done();
	}

	if (!GRHISupportsAsyncTextureCreation)
	{
		return Voxel::RenderTask([=]
		{
			VOXEL_FUNCTION_COUNTER();

			if (!TargetTexture.IsValid())
			{
				return FVoxelFuture::Done();
			}

			FTextureResource* Resource = TargetTexture->GetResource();
			if (!ensure(Resource))
			{
				return FVoxelFuture::Done();
			}

			const TRefCountPtr<FRHITexture> UploadTextureRHI = RHICreateTexture(
				FRHITextureCreateDesc::Create2D(TEXT("AsyncCopyTexture"))
				.SetExtent(SizeX, SizeY)
				.SetFormat(Format)
				.SetNumMips(1)
				.SetNumSamples(1)
				.SetFlags(TexCreate_ShaderResource));

			if (!ensure(UploadTextureRHI.IsValid()))
			{
				return FVoxelFuture::Done();
			}

			uint32 Stride = 0;
			void* LockedData = RHILockTexture2D(
				UploadTextureRHI,
				0,
				RLM_WriteOnly,
				Stride,
				false,
				false);

			if (ensure(LockedData))
			{
				FMemory::Memcpy(LockedData, Data->GetData(), Data->Num());
			}

			RHIUnlockTexture2D(UploadTextureRHI, 0, false, false);

			Resource->TextureRHI = UploadTextureRHI;

			struct FHack : FTextureResource
			{
				static FTextureReferenceRHIRef& GetReference(FTextureResource* Resource)
				{
					return static_cast<FHack*>(Resource)->TextureReferenceRHI;
				}
			};

			const FTextureReferenceRHIRef& TextureReferenceRHI = FHack::GetReference(Resource);
			if (ensure(TextureReferenceRHI.IsValid()))
			{
				VOXEL_SCOPE_COUNTER("RHIUpdateTextureReference");
				RHIUpdateTextureReference(TextureReferenceRHI, Resource->TextureRHI);
			}

			return FVoxelFuture::Done();
		});
	}

	return Voxel::AsyncTask([=]
	{
		VOXEL_FUNCTION_COUNTER();
		VOXEL_SCOPE_COUNTER("RHIAsyncCreateTexture2D");

		TArray<void*> MipData;
		MipData.Add(ConstCast(Data->GetData()));

		FGraphEventRef CompletionEvent;
		const FTextureRHIRef UploadTextureRHI = RHIAsyncCreateTexture2D(
			SizeX,
			SizeY,
			Format,
			1,
			TexCreate_ShaderResource,
			ERHIAccess::Unknown,
			MipData.GetData(),
			1,
			TEXT("AsyncCopyTexture"),
			CompletionEvent);

		if (CompletionEvent.IsValid())
		{
			VOXEL_SCOPE_COUNTER("Wait");
			CompletionEvent->Wait();
		}

		if (!ensure(UploadTextureRHI.IsValid()))
		{
			return FVoxelFuture::Done();
		}

		return Voxel::RenderTask([=]
		{
			UpdateTextureRef(TargetTexture.Get(), UploadTextureRHI);
		});
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelUtilities::ResetPreviousLocalToWorld(
	const UPrimitiveComponent& Component,
	const FPrimitiveSceneProxy& SceneProxy)
{
	// Hack to cancel motion blur when mesh components are reused in the same frame
	Voxel::RenderTask([&SceneProxy, PreviousLocalToWorld = Component.GetRenderMatrix()]
	{
		FScene& Scene = static_cast<FScene&>(SceneProxy.GetScene());
		Scene.VelocityData.OverridePreviousTransform(SceneProxy.GetPrimitiveComponentId(), PreviousLocalToWorld);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_PRIVATE_ACCESS(FRDGBuilder, Buffers, FRDGBufferRegistry);
DEFINE_PRIVATE_ACCESS(FRDGBuilder, Textures, FRDGTextureRegistry);

FRDGBufferRef FVoxelUtilities::FindBuffer(
	FRDGBuilder& GraphBuilder,
	const FString& Name)
{
	VOXEL_FUNCTION_COUNTER();

	FRDGBufferRef Result = nullptr;

	FRDGBufferRegistry& Buffers = PRIVATE_ACCESS_REF(FRDGBuilder, Buffers)(GraphBuilder);
	Buffers.Enumerate([&](FRDGBuffer* Buffer)
	{
		if (FStringView(Buffer->Name) != Name)
		{
			return;
		}

		ensure(!Result);
		Result = Buffer;
	});

	return Result;
}

FRDGTextureRef FVoxelUtilities::FindTexture(
	FRDGBuilder& GraphBuilder,
	const FString& Name)
{
	VOXEL_FUNCTION_COUNTER();

	FRDGTextureRef Result = nullptr;

	FRDGTextureRegistry& Textures = PRIVATE_ACCESS_REF(FRDGBuilder, Textures)(GraphBuilder);
	Textures.Enumerate([&](FRDGTexture* Texture)
	{
		if (FStringView(Texture->Name) != Name)
		{
			return;
		}

		ensure(!Result);
		Result = Texture;
	});

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BEGIN_SHADER_PARAMETER_STRUCT(FVoxelUtilitiesUploadParameters, )
	RDG_BUFFER_ACCESS(TargetBuffer, ERHIAccess::CopyDest)
END_SHADER_PARAMETER_STRUCT()

void FVoxelUtilities::UploadBuffer(
	FRDGBuilder& GraphBuilder,
	const FRDGBufferRef& TargetBuffer,
	const TConstVoxelArrayView64<uint8> Data,
	const FSharedVoidPtr& KeepAlive)
{
	if (!ensure(TargetBuffer) ||
		!ensure(TargetBuffer->Desc.NumElements * TargetBuffer->Desc.BytesPerElement >= Data.Num()))
	{
		return;
	}

	FVoxelUtilitiesUploadParameters* UploadParameters = GraphBuilder.AllocParameters<FVoxelUtilitiesUploadParameters>();
	UploadParameters->TargetBuffer = TargetBuffer;

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("FVoxelUtilities::UploadBuffer"),
		UploadParameters,
		ERDGPassFlags::Copy,
		[=](FRHICommandListImmediate& RHICmdList)
		{
			VOXEL_SCOPE_COUNTER_FORMAT("FVoxelUtilities::UploadBuffer %lldB", Data.Num());

			(void)KeepAlive;

			FRHIBuffer* TargetBufferRHI = TargetBuffer->GetRHI();

			void* TargetBufferData = RHICmdList.LockBuffer(TargetBufferRHI, 0, Data.Num(), RLM_WriteOnly);
			if (ensure(TargetBufferData))
			{
				FMemory::Memcpy(TargetBufferData, Data.GetData(), Data.Num());
			}
			RHICmdList.UnlockBuffer(TargetBufferRHI);
		});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelReadbackManager : public FVoxelSingleton
{
public:
	struct FReadback
	{
		TVoxelPromise<TVoxelArray64<uint8>> Promise;
		TSharedRef<FVoxelGPUBufferReadback> Readback;
	};
	TVoxelArray<FReadback> Readbacks_RenderThread;

	//~ Begin FVoxelSingleton Interface
	virtual void Tick_RenderThread(FRHICommandList& RHICmdList) override
	{
		VOXEL_FUNCTION_COUNTER();

		for (auto It = Readbacks_RenderThread.CreateIterator(); It; ++It)
		{
			if (!It->Readback->IsReady())
			{
				continue;
			}

			const TSharedRef<TVoxelArray64<uint8>> Array = MakeVoxelShared<TVoxelArray64<uint8>>(It->Readback->Lock());
			It->Readback->Unlock();

			It->Promise.Set(Array);
			It.RemoveCurrentSwap();
		}
	}
	//~ End FVoxelSingleton Interface
};
FVoxelReadbackManager* GVoxelReadbackManager = new FVoxelReadbackManager();

TVoxelFuture<TVoxelArray64<uint8>> FVoxelUtilities::Readback(
	const FBufferRHIRef& SourceBuffer,
	const int64 NumBytes)
{
	VOXEL_FUNCTION_COUNTER();

	if (!IsInRenderingThread())
	{
		return Voxel::RenderTask([=]
		{
			return Readback(SourceBuffer, NumBytes);
		});
	}
	check(IsInRenderingThread());

	const TVoxelPromise<TVoxelArray64<uint8>> Promise;

	GVoxelReadbackManager->Readbacks_RenderThread.Add(FVoxelReadbackManager::FReadback
	{
		Promise,
		FVoxelGPUBufferReadback::Create(FRHICommandListImmediate::Get(), SourceBuffer, NumBytes)
	});

	return Promise.GetFuture();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BEGIN_SHADER_PARAMETER_STRUCT(FVoxelUtilitiesReadbackParameters, )
	RDG_BUFFER_ACCESS(Buffer, ERHIAccess::CopySrc)
END_SHADER_PARAMETER_STRUCT()

TVoxelFuture<TVoxelArray64<uint8>> FVoxelUtilities::Readback(
	FRDGBuilder& GraphBuilder,
	const FRDGBufferRef& SourceBuffer,
	const int64 NumBytes)
{
	const TVoxelPromise<TVoxelArray64<uint8>> Promise;

	FVoxelUtilitiesReadbackParameters* Parameters = GraphBuilder.AllocParameters<FVoxelUtilitiesReadbackParameters>();
	Parameters->Buffer = SourceBuffer;

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("FVoxelUtilities::Readback"),
		Parameters,
		ERDGPassFlags::Readback,
		[SourceBuffer, NumBytes, Promise](FRHICommandList& RHICmdList)
		{
			Readback(SourceBuffer->GetRHI(), NumBytes).Then_AnyThread([Promise](const TSharedRef<TVoxelArray64<uint8>>& Data)
			{
				Promise.Set(Data);
			});
		});

	return Promise.GetFuture();
}