// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPrimitiveComponentSettings.generated.h"

USTRUCT(BlueprintType)
struct VOXELCORE_API FVoxelPrimitiveComponentSettings
{
	GENERATED_BODY()

	// Controls whether the foliage should cast a shadow or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bCastShadow = true;

	// Quality of indirect lighting for Movable primitives.  This has a large effect on Indirect Lighting Cache update time.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	TEnumAsByte<EIndirectLightingCacheQuality> IndirectLightingCacheQuality = ILCQ_Point;

	// Controls the type of lightmap used for this component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	ELightmapType LightmapType = ELightmapType::Default;

	// Whether the primitive will be used as an emissive light source.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bEmissiveLightSource = false;

	// Controls whether the primitive should inject light into the Light Propagation Volume.  This flag is only used if CastShadow is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow"))
	bool bAffectDynamicIndirectLighting = true;

	// Controls whether the primitive should affect indirect lighting when hidden. This flag is only used if bAffectDynamicIndirectLighting is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bAffectDynamicIndirectLighting"))
	bool bAffectIndirectLightingWhileHidden = false;

	// Controls whether the primitive should affect dynamic distance field lighting methods.  This flag is only used if CastShadow is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bAffectDistanceFieldLighting = false;

	// Controls whether the primitive should cast shadows in the case of non precomputed shadowing.  This flag is only used if CastShadow is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Dynamic Shadow"))
	bool bCastDynamicShadow = true;

	// Whether the object should cast a static shadow from shadow casting lights.  This flag is only used if CastShadow is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Static Shadow"))
	bool bCastStaticShadow = true;

	// Control shadow invalidation behavior, in particular with respect to Virtual Shadow Maps and material effects like World Position Offset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow"))
	EShadowCacheInvalidationBehavior ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Auto;

	// Whether the object should cast a volumetric translucent shadow.
	// Volumetric translucent shadows are useful for primitives with smoothly changing opacity like particles representing a volume,
	// But have artifacts when used on highly opaque surfaces.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Volumetric Translucent Shadow"))
	bool bCastVolumetricTranslucentShadow = false;

	// Whether the object should cast contact shadows.
	// This flag is only used if CastShadow is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Contact Shadow"))
	bool bCastContactShadow = true;

	// When enabled, the component will only cast a shadow on itself and not other components in the world.
	// This is especially useful for first person weapons, and forces bCastInsetShadow to be enabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow"))
	bool bSelfShadowOnly = false;

	// When enabled, the component will be rendering into the far shadow cascades (only for directional lights).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Far Shadow"))
	bool bCastFarShadow = false;

	// Whether this component should create a per-object shadow that gives higher effective shadow resolution.
	// Useful for cinematic character shadowing. Assumed to be enabled if bSelfShadowOnly is enabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Dynamic Inset Shadow"))
	bool bCastInsetShadow = false;

	// Whether this component should cast shadows from lights that have bCastShadowsFromCinematicObjectsOnly enabled.
	// This is useful for characters in a cinematic with special cinematic lights, where the cost of shadowmap rendering of the environment is undesired.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow"))
	bool bCastCinematicShadow = false;

	// If true, the primitive will cast shadows even if bHidden is true.
	// Controls whether the primitive should cast shadows when hidden.
	// This flag is only used if CastShadow is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Hidden Shadow"))
	bool bCastHiddenShadow = false;

	// Whether this primitive should cast dynamic shadows as if it were a two sided material.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bCastShadow", DisplayName = "Shadow Two Sided"))
	bool bCastShadowAsTwoSided = false;

	// Whether to light this component and any attachments as a group.  This only has effect on the root component of an attachment tree.
	// When enabled, attached component shadowing settings like bCastInsetShadow, bCastVolumetricTranslucentShadow, etc, will be ignored.
	// This is useful for improving performance when multiple movable components are attached together.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bLightAttachmentsAsGroup = false;

	// If set, then it overrides any bLightAttachmentsAsGroup set in a parent.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bExcludeFromLightAttachmentGroup = false;

	// Whether the whole component should be shadowed as one from stationary lights, which makes shadow receiving much cheaper.
	// When enabled shadowing data comes from the volume lighting samples precomputed by Lightmass, which are very sparse.
	// This is currently only used on stationary directional lights.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bSingleSampleShadowFromStationaryLights = false;

	// Lighting channels that placed foliage will be assigned. Lights with matching channels will affect the foliage.
	// These channels only apply to opaque materials, direct lighting, and dynamic lighting and shadowing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FLightingChannels LightingChannels;

	// If true, this component will be visible in reflection captures.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bVisibleInReflectionCaptures = true;

	// If true, this component will be visible in real-time sky light reflection captures.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bVisibleInRealTimeSkyCaptures = true;

	// If true, this component will be visible in ray tracing effects. Turning this off will remove it from ray traced reflections, shadows, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bVisibleInRayTracing = true;

	// If true, this component will be rendered in the main pass (z prepass, basepass, transparency)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bRenderInMainPass = true;

	// If true, this component will be rendered in the depth pass even if it's not rendered in the main pass
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (EditCondition = "!bRenderInMainPass"))
	bool bRenderInDepthPass = true;

	// Whether the primitive receives decals.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bReceivesDecals = true;

	// If this is True, this component won't be visible when the view actor is the component's owner, directly or indirectly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bOwnerNoSee = false;

	// If this is True, this component will only be visible when the view actor is the component's owner, directly or indirectly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bOnlyOwnerSee = false;

	// Treat this primitive as part of the background for occlusion purposes. This can be used as an optimization to reduce the cost of rendering skyboxes, large ground planes that are part of the vista, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bTreatAsBackgroundForOcclusion = false;

	// Whether to render the primitive in the depth only pass.
	// This should generally be true for all objects, and let the renderer make decisions about whether to render objects in the depth only pass.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bUseAsOccluder = true;

	// If true, this component will be rendered in the CustomDepth pass (usually used for outlines)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (DisplayName = "Render CustomDepth Pass"))
	bool bRenderCustomDepth = false;

	// Optionally write this 0-255 value to the stencil buffer in CustomDepth pass (Requires project setting or r.CustomDepth == 3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = "0", UIMax = "255", EditCondition = "bRenderCustomDepth", DisplayName = "CustomDepth Stencil Value"))
	int32 CustomDepthStencilValue = 0;

	// When true, will only be visible in Scene Capture
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (DisplayName = "Visible In Scene Capture Only"))
	bool bVisibleInSceneCaptureOnly = false;

	// When true, will not be captured by Scene Capture
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (DisplayName = "Hidden In Scene Capture"))
	bool bHiddenInSceneCapture = false;

	// Translucent objects with a lower sort priority draw behind objects with a higher priority.
	// Translucent objects with the same priority are rendered from back-to-front based on their bounds origin.
	// This setting is also used to sort objects being drawn into a runtime virtual texture.
	// Ignored if the object is not translucent.  The default priority is zero.
	// Warning: This should never be set to a non-default value unless you know what you are doing, as it will prevent the renderer from sorting correctly.
	// It is especially problematic on dynamic gameplay effects.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	int32 TranslucencySortPriority = 0;

	// Modified sort distance offset for translucent objects in world units.
	// A positive number will move the sort distance further and a negative number will move the distance closer.
	// Ignored if the object is not translucent.
	// Warning: Adjusting this value will prevent the renderer from correctly sorting based on distance.  Only modify this value if you are certain it will not cause visual artifacts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	float TranslucencySortDistanceOffset = 0.f;

	// Mask used for stencil buffer writes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (EditCondition = "bRenderCustomDepth"))
	ERendererStencilMask CustomDepthStencilWriteMask = ERendererStencilMask::ERSM_Default;

	// Array of runtime virtual textures into which we draw the mesh for this actor.
	// The material also needs to be set up to output to a virtual texture.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Texture", meta = (DisplayName = "Draw in Virtual Textures"))
	TArray<TObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextures;

	// Bias to the LOD selected for rendering to runtime virtual textures.
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Virtual Texture", meta = (DisplayName = "Virtual Texture LOD Bias", UIMin = "-7", UIMax = "8"))
	int8 VirtualTextureLodBias = 0;

	// Number of lower mips in the runtime virtual texture to skip for rendering this primitive.
	// Larger values reduce the effective draw distance in the runtime virtual texture.
	// This culling method doesn't take into account primitive size or virtual texture size.
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Virtual Texture", meta = (DisplayName = "Virtual Texture Skip Mips", UIMin = "0", UIMax = "7"))
	int8 VirtualTextureCullMips = 0;

	// Set the minimum pixel coverage before culling from the runtime virtual texture.
	// Larger values reduce the effective draw distance in the runtime virtual texture.
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Virtual Texture", meta = (UIMin = "0", UIMax = "7"))
	int8 VirtualTextureMinCoverage = 0;

	// Controls if this component draws in the main pass as well as in the virtual texture.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Virtual Texture", meta = (DisplayName = "Draw in Main Pass"))
	ERuntimeVirtualTextureMainPassType VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;

public:
	void ApplyToComponent(UPrimitiveComponent& Component) const;
	bool operator==(const FVoxelPrimitiveComponentSettings& Other) const;
};