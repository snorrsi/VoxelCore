﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

template<typename...>
struct TVoxelTypes;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename>
struct TVoxelFunctionInfo;

template<typename InReturnType, typename... InArgTypes>
struct TVoxelFunctionInfo<InReturnType(InArgTypes...)>
{
	using ReturnType = InReturnType;
	using ArgTypes = TVoxelTypes<InArgTypes...>;
	using Signature = InReturnType(InArgTypes...);
};

template<typename ReturnType, typename Class, typename... ArgTypes>
struct TVoxelFunctionInfo<ReturnType(Class::*)(ArgTypes...) const> : TVoxelFunctionInfo<ReturnType(ArgTypes...)>
{
};

template<typename ReturnType, typename Class, typename... ArgTypes>
struct TVoxelFunctionInfo<ReturnType(Class::*)(ArgTypes...)> : TVoxelFunctionInfo<ReturnType(ArgTypes...)>
{
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename LambdaType, typename = void>
struct TVoxelLambdaInfo : TVoxelFunctionInfo<void()>
{
	static_assert(sizeof(LambdaType) == -1, "LambdaType is not a lambda. Note that generic lambdas (eg [](auto)) are not supported.");
};

template<typename LambdaType>
struct TVoxelLambdaInfo<LambdaType, std::enable_if_t<sizeof(decltype(&LambdaType::operator())) != 0>> : TVoxelFunctionInfo<decltype(&LambdaType::operator())>
{
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename Signature>
using FunctionReturnType_T = typename TVoxelFunctionInfo<Signature>::ReturnType;

template<typename Signature>
using FunctionArgTypes_T = typename TVoxelFunctionInfo<Signature>::ArgTypes;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename LambdaType>
using LambdaReturnType_T = typename TVoxelLambdaInfo<LambdaType>::ReturnType;

template<typename LambdaType>
using LambdaArgTypes_T = typename TVoxelLambdaInfo<LambdaType>::ArgTypes;

template<typename LambdaType>
using LambdaSignature_T = typename TVoxelLambdaInfo<LambdaType>::Signature;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename LambdaType, typename Signature>
constexpr bool LambdaHasSignature_V =
	std::is_same_v<LambdaReturnType_T<LambdaType>, FunctionReturnType_T<Signature>> &&
	std::is_same_v<LambdaArgTypes_T<LambdaType>, FunctionArgTypes_T<Signature>>;

template<typename LambdaType, typename Signature>
using LambdaHasSignature_T = std::enable_if_t<LambdaHasSignature_V<LambdaType, Signature>>;

template<typename LambdaType, typename Type>
using LambdaDependentType_T = typename TChooseClass<std::is_same_v<LambdaType, void>, void, Type>::Result;