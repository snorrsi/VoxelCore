﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

extern VOXELCORE_API IVoxelTaskDispatcher* GVoxelGlobalTaskDispatcher;

class VOXELCORE_API FVoxelTaskDispatcherRef
{
public:
	FVoxelTaskDispatcherRef() = default;
	FVoxelTaskDispatcherRef(const IVoxelTaskDispatcher& Dispatcher);

	TSharedPtr<IVoxelTaskDispatcher> Pin() const;

	FORCEINLINE bool IsNull() const
	{
		checkVoxelSlow((Index == -1) == (Serial == -1));
		return Index == -1;
	}
	FORCEINLINE bool operator==(const FVoxelTaskDispatcherRef& Other) const
	{
		return
			Index == Other.Index &&
			Serial == Other.Serial;
	}

private:
	int32 Index = -1;
	int32 Serial = -1;

	friend IVoxelTaskDispatcher;
};

class VOXELCORE_API IVoxelTaskDispatcher : public TSharedFromThis<IVoxelTaskDispatcher>
{
public:
	const bool bTrackPromisesCallstacks;

	explicit IVoxelTaskDispatcher(const bool bTrackPromisesCallstacks)
		: bTrackPromisesCallstacks(bTrackPromisesCallstacks)
	{
	}
	virtual ~IVoxelTaskDispatcher();

	VOXEL_COUNT_INSTANCES();

public:
	virtual void DispatchImpl(
		EVoxelFutureThread Thread,
		TVoxelUniqueFunction<void()> Lambda) = 0;

	virtual bool IsExiting() const = 0;

public:
	void DumpPromises();

	void Dispatch(
		EVoxelFutureThread Thread,
		TVoxelUniqueFunction<void()> Lambda);

public:
	FORCEINLINE int32 GetNumPromises() const
	{
		return NumPromises.Get();
	}

	// Wrap another future created in a different task dispatcher
	// This ensures any continuation to this future won't leak to the other task dispatcher,
	// which would mess up task tracking
	FVoxelFuture Wrap(const FVoxelFuture& Other)
	{
		FVoxelPromise Promise(this);
		Promise.Set(Other);
		return Promise;
	}
	template<typename T>
	TVoxelFuture<T> Wrap(const TVoxelFuture<T>& Other)
	{
		TVoxelPromise<T> Promise(this);
		Promise.Set(Other);
		return Promise;
	}

private:
	TVoxelAtomic<FVoxelTaskDispatcherRef> SelfRef;
	FVoxelCounter32_WithPadding NumPromises;

	FVoxelCriticalSection CriticalSection;
	TVoxelSparseArray<FVoxelStackFrames> StackFrames_RequiresLock;
	TVoxelChunkedSparseArray<TSharedPtr<FVoxelPromiseState>> PromisesToKeepAlive_RequiresLock;

	friend FVoxelPromiseState;
	friend FVoxelTaskDispatcherRef;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VOXELCORE_API const uint32 GVoxelTaskDispatcherScopeTLS;

class VOXELCORE_API FVoxelTaskDispatcherScope
{
public:
	FORCEINLINE explicit FVoxelTaskDispatcherScope(IVoxelTaskDispatcher& Dispatcher)
		: Dispatcher(Dispatcher)
		, PreviousTLS(FPlatformTLS::GetTlsValue(GVoxelTaskDispatcherScopeTLS))
	{
		FPlatformTLS::SetTlsValue(GVoxelTaskDispatcherScopeTLS, &Dispatcher);
	}
	FORCEINLINE ~FVoxelTaskDispatcherScope()
	{
		checkVoxelSlow(FPlatformTLS::GetTlsValue(GVoxelTaskDispatcherScopeTLS) == &Dispatcher);
		FPlatformTLS::SetTlsValue(GVoxelTaskDispatcherScopeTLS, PreviousTLS);
	}

	FORCEINLINE static IVoxelTaskDispatcher& Get()
	{
		if (IVoxelTaskDispatcher* Dispatcher = static_cast<IVoxelTaskDispatcher*>(FPlatformTLS::GetTlsValue(GVoxelTaskDispatcherScopeTLS)))
		{
			return *Dispatcher;
		}

		return *GVoxelGlobalTaskDispatcher;
	}

public:
	// Call the lambda in the global task dispatcher scope, avoiding any task leak or weird dependencies
	template<
		typename LambdaType,
		typename T = LambdaReturnType_T<LambdaType>>
	static TVoxelFutureType<T> CallInGlobalScope(LambdaType Lambda)
	{
		return Get().Wrap(INLINE_LAMBDA
		{
			FVoxelTaskDispatcherScope Scope(*GVoxelGlobalTaskDispatcher);
			return Lambda();
		});
	}

private:
	IVoxelTaskDispatcher& Dispatcher;
	void* const PreviousTLS;
};