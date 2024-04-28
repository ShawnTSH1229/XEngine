#pragma once
#include "PlatformRHI.h"
#include "Runtime/HAL/Mch.h"
#include "Runtime/RenderCore/RenderResource.h"

template<typename InitializerType, typename RHIStateType>
class TStaticStateRHI 
{

private:
	class XStaticStateResource : public XRenderResource
	{
	public:
		std::shared_ptr<RHIStateType>StateRHI;
		XStaticStateResource()
		{
			if (GPlatformRHI != nullptr)
			{
				StateRHI = InitializerType::CreateRHI();
			}
			else
			{
				BeginInitResource(this);
			}
		}
		void InitRHI()override
		{
			StateRHI = InitializerType::CreateRHI();
		}
		void ReleaseRHI() override {}

	};
	static XStaticStateResource StaticResource;
public:
	static RHIStateType* GetRHI()
	{
		XASSERT(StaticResource.StateRHI.get() != nullptr);
		return StaticResource.StateRHI.get();
	};
};

//template<typename T>
//double S<T>::something_relevant = 1.5;

template<typename InitializerType, typename RHIStateType>
typename 
TStaticStateRHI<InitializerType, RHIStateType>::XStaticStateResource
TStaticStateRHI<InitializerType, RHIStateType>::StaticResource =
TStaticStateRHI<InitializerType, RHIStateType>::XStaticStateResource();


template<bool bConservative = false, EFaceCullMode CullMode = EFaceCullMode::FC_Back>
class TStaticRasterizationState :
	public TStaticStateRHI<TStaticRasterizationState<bConservative, CullMode>,
	XRHIRasterizationState>
{
public:
	static std::shared_ptr<XRHIRasterizationState> CreateRHI()
	{
		XRasterizationStateInitializerRHI Initializer(bConservative, CullMode);
		return RHICreateRasterizationStateState(Initializer);
	}
};

template<bool bEnableDepthWrite = true, ECompareFunction DepthTest = ECompareFunction::CF_Greater>
class TStaticDepthStencilState:
	public TStaticStateRHI<TStaticDepthStencilState<bEnableDepthWrite, DepthTest>, 
	XRHIDepthStencilState>
{
public:
	static std::shared_ptr<XRHIDepthStencilState> CreateRHI()
	{
		XDepthStencilStateInitializerRHI Initializer(bEnableDepthWrite, DepthTest);
		return RHICreateDepthStencilState(Initializer);
	}
};

template<bool RT0BlendeEnable = false,
	EBlendOperation	RT0ColorBlendOp = EBlendOperation::BO_Add,
	EBlendFactor    RT0ColorSrcBlend = EBlendFactor::BF_One,
	EBlendFactor    RT0ColorDestBlend = EBlendFactor::BF_Zero,
	EBlendOperation RT0AlphaBlendOp = EBlendOperation::BO_Add,
	EBlendFactor    RT0AlphaSrcBlend = EBlendFactor::BF_One,
	EBlendFactor    RT0AlphaDestBlend = EBlendFactor::BF_Zero,
	bool RT1BlendeEnable = false,
	bool RT2BlendeEnable = false,
	bool RT3BlendeEnable = false,
	bool RT4BlendeEnable = false,
	bool RT5BlendeEnable = false,
	bool RT6BlendeEnable = false,
	bool RT7BlendeEnable = false>
	class TStaticBlendState :public TStaticStateRHI<TStaticBlendState<
	RT0BlendeEnable,
	RT0ColorBlendOp,
	RT0ColorSrcBlend,
	RT0ColorDestBlend,
	RT0AlphaBlendOp,
	RT0AlphaSrcBlend,
	RT0AlphaDestBlend,
	RT1BlendeEnable,
	RT2BlendeEnable,
	RT3BlendeEnable,
	RT4BlendeEnable,
	RT5BlendeEnable,
	RT6BlendeEnable,
	RT7BlendeEnable>, XRHIBlendState>
{
public:
	static std::shared_ptr<XRHIBlendState> CreateRHI()
	{
		std::array<XBlendStateInitializerRHI::XRenderTarget, 8>RenderTargets;
		RenderTargets[0] = XBlendStateInitializerRHI::XRenderTarget(
			RT0BlendeEnable,
			RT0ColorBlendOp,
			RT0ColorSrcBlend,
			RT0ColorDestBlend,
			RT0AlphaBlendOp,
			RT0AlphaSrcBlend,
			RT0AlphaDestBlend);
		return RHICreateBlendState(RenderTargets);
	}
};