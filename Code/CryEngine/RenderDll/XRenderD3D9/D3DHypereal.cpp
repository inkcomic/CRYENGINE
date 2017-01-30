// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#if defined(INCLUDE_VR_RENDERING)

	#include "D3DHypereal.h"
	#include "DriverD3D.h"
	#include "D3DPostProcess.h"
	#include "DeviceInfo.h"

	#include <CrySystem/VR/IHMDManager.h>
	#include <CrySystem/VR/IHMDDevice.h>
	#ifdef ENABLE_BENCHMARK_SENSOR
		#include <IBenchmarkFramework.h>
		#include <IBenchmarkRendererSensorManager.h>
	#endif

CD3DHyperealRenderer::CD3DHyperealRenderer(CryVR::Hypereal::IHyperealDevice* hyperealDevice, CD3D9Renderer* renderer, CD3DStereoRenderer* stereoRenderer)
	: m_pHyperealDevice(hyperealDevice)
	, m_pRenderer(renderer)
	, m_pStereoRenderer(stereoRenderer)
	, m_eyeWidth(~0L)
	, m_eyeHeight(~0L)
	, m_numFrames(0)
	, m_currentFrame(0)
{
	ZeroArray(m_scene3DRenderData);
	ZeroArray(m_quadLayerRenderData);
	ZeroArray(m_mirrorTextures);

	for (uint32 i = RenderLayer::eQuadLayers_0; i < RenderLayer::eQuadLayers_Total; ++i)
	{
		m_quadLayerProperties[i].SetType(RenderLayer::eLayer_Quad);
		m_quadLayerProperties[i].SetPose(QuatTS(Quat(IDENTITY), Vec3(0.f, 0.f, -0.6f), 1.f));
		m_quadLayerProperties[i].SetId(i);
	}

	m_Param0Name = CCryNameR("texToTexParams0");
	m_Param1Name = CCryNameR("texToTexParams1");
	m_textureToTexture = CCryNameTSCRC("TextureToTexture");
}

CD3DHyperealRenderer::~CD3DHyperealRenderer()
{
}

bool CD3DHyperealRenderer::Initialize()
{
	D3DDevice* d3d11Device = m_pRenderer->GetDevice_Unsynchronized().GetRealDevice();

	m_pHyperealDevice->CreateGraphicsContext(d3d11Device);

	m_pHyperealDevice->GetRenderTargetSize(m_eyeWidth, m_eyeHeight);//1200x1080??
// 	m_eyeWidth = m_pRenderer->GetWidth();
// 	m_eyeHeight = m_pRenderer->GetHeight();

	CryVR::Hypereal::TextureDesc eyeTextureDesc;
	eyeTextureDesc.width = m_eyeWidth;
	eyeTextureDesc.height = m_eyeHeight;
	eyeTextureDesc.format = (uint32)DXGI_FORMAT_R8G8B8A8_UNORM;

	CryVR::Hypereal::TextureDesc quadTextureDesc;
	quadTextureDesc.width = m_eyeWidth;
	quadTextureDesc.height = m_eyeHeight;
	quadTextureDesc.format = (uint32)DXGI_FORMAT_R8G8B8A8_UNORM;

	CryVR::Hypereal::TextureDesc mirrorTextureDesc;
	mirrorTextureDesc.width = m_eyeWidth * 2;
	mirrorTextureDesc.height = m_eyeHeight;
	mirrorTextureDesc.format = (uint32)DXGI_FORMAT_R8G8B8A8_UNORM;

	if (!InitializeEyeTarget(d3d11Device, EEyeType::eEyeType_LeftEye, eyeTextureDesc, "$LeftEye") ||
	    !InitializeEyeTarget(d3d11Device, EEyeType::eEyeType_RightEye, eyeTextureDesc, "$RightEye") ||
	    !InitializeQuadLayer(d3d11Device, RenderLayer::eQuadLayers_0, quadTextureDesc, "$QuadLayer0") ||
	    !InitializeQuadLayer(d3d11Device, RenderLayer::eQuadLayers_1, quadTextureDesc, "$QuadLayer1") ||
	    !InitializeMirrorTexture(d3d11Device, EEyeType::eEyeType_LeftEye, mirrorTextureDesc, "$LeftMirror") ||
	    !InitializeMirrorTexture(d3d11Device, EEyeType::eEyeType_RightEye, mirrorTextureDesc, "$RightMirror"))
	{
		CryLogAlways("[HMD][Hypereal] Texture creation failed");
		Shutdown();
		return false;
	}
	
	// Scene3D layers
	m_pHyperealDevice->OnSetupEyeTargets(
		CryVR::Hypereal::ERenderAPI::eRenderAPI_DirectX,
		CryVR::Hypereal::ERenderColorSpace::eRenderColorSpace_Auto,
		m_scene3DRenderData[EEyeType::eEyeType_LeftEye].texture->GetDevTexture()->Get2DTexture(),
		m_scene3DRenderData[EEyeType::eEyeType_RightEye].texture->GetDevTexture()->Get2DTexture()
	);

	// Quad layers
	for (int i = 0; i < RenderLayer::eQuadLayers_Total; i++)
	{
		m_pHyperealDevice->OnSetupOverlay(i,m_quadLayerRenderData[i].texture->GetDevTexture()->Get2DTexture());
	}

	// Mirror texture
	void* srv = nullptr;
	for (uint32 eye = 0; eye < 2; ++eye)
	{
		// Request the resource-view from openVR
		m_pHyperealDevice->GetMirrorImageView(static_cast<EEyeType>(eye), m_mirrorTextures[eye]->GetDevTexture()->Get2DTexture(), &srv);
		m_mirrorTextures[eye]->SetShaderResourceView(static_cast<D3DShaderResource*>(srv), false);
	}

	return true;
}

bool CD3DHyperealRenderer::InitializeEyeTarget(D3DDevice* d3d11Device, EEyeType eye, CryVR::Hypereal::TextureDesc desc, const char* name)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = desc.width;
	textureDesc.Height = desc.height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = (DXGI_FORMAT)desc.format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	ID3D11Texture2D* texture;
	d3d11Device->CreateTexture2D(&textureDesc, nullptr, &texture);

	ETEX_Format format = CTexture::TexFormatFromDeviceFormat((DXGI_FORMAT)desc.format);
	m_scene3DRenderData[eye].texture = m_pStereoRenderer->WrapD3DRenderTarget(static_cast<D3DTexture*>(texture), desc.width, desc.height, format, name, true);

	return true;
}

bool CD3DHyperealRenderer::InitializeQuadLayer(D3DDevice* d3d11Device, RenderLayer::EQuadLayers quadLayer, CryVR::Hypereal::TextureDesc desc, const char* name)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = desc.width;
	textureDesc.Height = desc.height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = (DXGI_FORMAT)desc.format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	ID3D11Texture2D* texture;
	d3d11Device->CreateTexture2D(&textureDesc, nullptr, &texture);

	char textureName[16];
	cry_sprintf(textureName, name, quadLayer);

	ETEX_Format format = CTexture::TexFormatFromDeviceFormat((DXGI_FORMAT)desc.format);
	m_quadLayerRenderData[quadLayer].texture = m_pStereoRenderer->WrapD3DRenderTarget(static_cast<D3DTexture*>(texture), desc.width, desc.height, format, textureName, true);

	return true;
}

bool CD3DHyperealRenderer::InitializeMirrorTexture(D3DDevice* d3d11Device, EEyeType eye, CryVR::Hypereal::TextureDesc desc, const char* name)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = desc.width;
	textureDesc.Height = desc.height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = (DXGI_FORMAT)desc.format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	ID3D11Texture2D* texture;
	d3d11Device->CreateTexture2D(&textureDesc, nullptr, &texture);

	ETEX_Format format = CTexture::TexFormatFromDeviceFormat((DXGI_FORMAT)desc.format);
	m_mirrorTextures[eye] = m_pStereoRenderer->WrapD3DRenderTarget(static_cast<D3DTexture*>(texture), desc.width, desc.height, format, name, false);

	return true;
}

void CD3DHyperealRenderer::Shutdown()
{
	m_pStereoRenderer->SetEyeTextures(nullptr, nullptr);

	// Scene3D layers
	for (uint32 eye = 0; eye < 2; ++eye)
	{
		m_scene3DRenderData[eye].texture->ReleaseForce();
		m_scene3DRenderData[eye].texture = nullptr;
	}

	// Quad layers
	for (uint32 i = 0; i < RenderLayer::eQuadLayers_Total; ++i)
	{
		m_pHyperealDevice->OnDeleteOverlay(i);
		m_quadLayerRenderData[i].texture->ReleaseForce();
		m_quadLayerRenderData[i].texture = nullptr;
	}

	// Mirror texture
	for (uint32 eye = 0; eye < 2; ++eye)
	{
		if (m_mirrorTextures[eye] != nullptr)
		{
			m_mirrorTextures[eye]->ReleaseForce();
			m_mirrorTextures[eye] = nullptr;
		}
	}

	ReleaseBuffers();

	m_pHyperealDevice->ReleaseGraphicsContext();
}

void CD3DHyperealRenderer::OnResolutionChanged()
{
	if (m_eyeWidth != m_pRenderer->GetWidth() ||
	    m_eyeHeight != m_pRenderer->GetHeight())
	{
		Shutdown();
		Initialize();
	}
}

void CD3DHyperealRenderer::ReleaseBuffers()
{
}

void CD3DHyperealRenderer::PrepareFrame()
{
	// Scene3D layer
	m_pStereoRenderer->SetEyeTextures(m_scene3DRenderData[0].texture, m_scene3DRenderData[1].texture);

	// Quad layers
	for (int i = 0; i < RenderLayer::eQuadLayers_Total; i++)
		m_pStereoRenderer->SetVrQuadLayerTexture(static_cast<RenderLayer::EQuadLayers>(i), m_quadLayerRenderData[i].texture);

}

void CD3DHyperealRenderer::SubmitFrame()
{
	#ifdef ENABLE_BENCHMARK_SENSOR
	gcpRendD3D->m_benchmarkRendererSensor->PreStereoFrameSubmit(m_scene3DRenderData[0].texture, m_scene3DRenderData[1].texture);
#endif

	UpdateLayers();
	CryVR::Hypereal::SHmdSubmitFrameData submitData;
	submitData.pQuadLayersArray = &m_renderLayerInfoArray[0];
	submitData.numQuadLayersArray = RenderLayer::eQuadLayers_Total;


	// Pass the final images and layer configuration to the Hypereal device
	m_pHyperealDevice->SubmitFrame(submitData);

	#ifdef ENABLE_BENCHMARK_SENSOR
	gcpRendD3D->m_benchmarkRendererSensor->AfterStereoFrameSubmit();
	#endif

	// Deactivate layers
	GetQuadLayerProperties(RenderLayer::eQuadLayers_0)->SetActive(false);
	GetQuadLayerProperties(RenderLayer::eQuadLayers_1)->SetActive(false);
}

void CD3DHyperealRenderer::RenderSocialScreen()
{
	CTexture* pBackbufferTexture = gcpRendD3D->GetBackBufferTexture();
	
	if (const IHmdManager* pHmdManager = gEnv->pSystem->GetHmdManager())
	{
		if (const IHmdDevice* pDev = pHmdManager->GetHmdDevice())
		{
			bool bKeepAspect = false;
			const EHmdSocialScreen socialScreen = pDev->GetSocialScreenType(&bKeepAspect);
			switch (socialScreen)
			{
			case EHmdSocialScreen::Off:
				// TODO: Clear backbuffer texture? Show a selected wallpaper?
				GetUtils().ClearScreen(0.1f, 0.1f, 0.1f, 1.0f); // we don't want true black, to distinguish between rendering error and no-social-screen. NOTE: THE CONSOLE WILL NOT BE DISPLAYED!!!
				break;
			// intentional fall through
			case EHmdSocialScreen::UndistortedLeftEye:
			case EHmdSocialScreen::UndistortedRightEye:
			case EHmdSocialScreen::UndistortedDualImage:
			{
				static CCryNameTSCRC pTechTexToTex("TextureToTexture");
				//const auto frameData = m_layerManager.ConstructFrameData();
				const bool bRenderBothEyes = socialScreen == EHmdSocialScreen::UndistortedDualImage;

				int eyesToRender[2] = { LEFT_EYE, -1 };
				if (socialScreen == EHmdSocialScreen::UndistortedRightEye)  eyesToRender[0] = RIGHT_EYE;
				else if (socialScreen == EHmdSocialScreen::UndistortedDualImage)
					eyesToRender[1] = RIGHT_EYE;

				uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
				gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5] | g_HWSR_MaskBit[HWSR_REVERSE_DEPTH]);

				int iTempX, iTempY, iWidth, iHeight;
				gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

				if (bKeepAspect)
				{
					gcpRendD3D->FX_ClearTarget(pBackbufferTexture, Clr_Empty);
				}

				gcpRendD3D->FX_PushRenderTarget(0, pBackbufferTexture, nullptr);
				gcpRendD3D->FX_SetActiveRenderTargets();

				for (int i = 0; i < CRY_ARRAY_COUNT(eyesToRender); ++i)
				{
					if (eyesToRender[i] < 0)
						continue;

					const auto pTex = eyesToRender[i] == LEFT_EYE ? m_scene3DRenderData[EEyeType::eEyeType_LeftEye].texture : m_scene3DRenderData[EEyeType::eEyeType_RightEye].texture;

					Vec2 targetSize;
					targetSize.x = pBackbufferTexture->GetWidth() * (bRenderBothEyes ? 0.5f : 1.0f);
					targetSize.y = float(pBackbufferTexture->GetHeight());

					Vec2 srcToTargetScale;
					srcToTargetScale.x = targetSize.x / pTex->GetWidth();
					srcToTargetScale.y = targetSize.y / pTex->GetHeight();

					if (bKeepAspect)
					{
						float minScale = min(srcToTargetScale.x, srcToTargetScale.y);

						srcToTargetScale.x = minScale;
						srcToTargetScale.y = minScale;
					}

					auto rescaleViewport = [&](const Vec2i& viewportPosition, const Vec2i& viewportSize)
					{
						Vec4 result;
						result.x = viewportPosition.x * srcToTargetScale.x;
						result.y = viewportPosition.y * srcToTargetScale.y;
						result.z = viewportSize.x * srcToTargetScale.x;
						result.w = viewportSize.y * srcToTargetScale.y;

						// shift to center
						Vec2 emptySpace;
						emptySpace.x = targetSize.x - result.z;
						emptySpace.y = targetSize.y - result.w;

						if (bRenderBothEyes)
						{
							result.x += (i == 0) ? emptySpace.x : targetSize.x;
							result.y += (i == 0) ? emptySpace.y : 0;
						}
						else if (!bRenderBothEyes)
						{
							result.x += 0.5f * emptySpace.x;
							result.y += 0.5f * emptySpace.y;
						}

						return result;
					};

					// draw eye texture first
					if (CTexture* pSrcTex = m_pStereoRenderer->GetEyeTarget(StereoEye(eyesToRender[i])))
					{
						auto vp = rescaleViewport(Vec2i(0,0), Vec2i(pTex->GetWidth(), pTex->GetHeight()));
						gcpRendD3D->RT_SetViewport(int(vp.x), int(vp.y), int(vp.z), int(vp.w));

						GetUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechTexToTex, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

						gRenDev->FX_SetState(GS_NODEPTHTEST);
						pSrcTex->Apply(0, CTexture::GetTexState(STexState(FILTER_LINEAR, true)));

						GetUtils().DrawFullScreenTri(0, 0);
						GetUtils().ShEndPass();
					}

					// now draw quad layers
					for (int i = 0; i < RenderLayer::eQuadLayers_Total; ++i)
					{
						const auto& quadLayer = m_quadLayerProperties[i];
						if (quadLayer.IsActive())
						{
							if (auto pQuadTex = m_pStereoRenderer->GetVrQuadLayerTex(RenderLayer::EQuadLayers(i)))
							{
								QuatTS tsLayer = quadLayer.GetPose();
								Vec2i viewportPosition(tsLayer.t.x, tsLayer.t.y);
								Vec2i viewportSize(quadLayer.GetTexture()->GetWidth(), quadLayer.GetTexture()->GetHeight());
								auto vp = rescaleViewport(viewportPosition, viewportSize);
								gcpRendD3D->RT_SetViewport(int(vp.x), int(vp.y), int(vp.z), int(vp.w));

								GetUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechTexToTex, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

								gRenDev->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
								pQuadTex->Apply(0, CTexture::GetTexState(STexState(FILTER_LINEAR, true)));

								GetUtils().DrawFullScreenTri(0, 0);
								GetUtils().ShEndPass();
							}
						}
					}
				}

				// Restore previous viewport
				gcpRendD3D->FX_PopRenderTarget(0);
				gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

				gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
			}
			break;
			case EHmdSocialScreen::DistortedDualImage:
			default:
			{
				if (pBackbufferTexture->GetDevTexture()->Get2DTexture() != nullptr)
				{
					m_pHyperealDevice->CopyMirrorImage(pBackbufferTexture->GetDevTexture()->Get2DTexture(), pBackbufferTexture->GetWidth(), pBackbufferTexture->GetHeight());
				}
			}
			break;
			}
		}
	}
}

RenderLayer::CProperties* CD3DHyperealRenderer::GetQuadLayerProperties(RenderLayer::EQuadLayers id)
{
	if (id < RenderLayer::eQuadLayers_Total)
	{
		return &(m_quadLayerProperties[id]);
	}
	return nullptr;
}

void CD3DHyperealRenderer::UpdateLayers()
{
	for (uint32 i = RenderLayer::EQuadLayers::eQuadLayers_0; i < RenderLayer::EQuadLayers::eQuadLayers_Total; ++i)
	{
		ITexture* pTexture = m_quadLayerProperties[i].GetTexture();
		if (pTexture/* && m_quadLayerProperties[i].IsActive()*/)
		{
			CTexture* pQuadTex = m_pStereoRenderer->GetVrQuadLayerTex((RenderLayer::EQuadLayers)i);
			GetUtils().StretchRect(static_cast<CTexture*>(pTexture), pQuadTex);
		}
	}

	// Quad layers
	for (uint32 i = 0; i < RenderLayer::eQuadLayers_Total; ++i)
	{
		if (GetQuadLayerProperties(static_cast<RenderLayer::EQuadLayers>(i))->IsActive())
		{
			m_pHyperealDevice->SubmitOverlay(i);
		}
	}

	// Quad layers
	for (uint32 i = 0; i < RenderLayer::eQuadLayers_Total; ++i)
	{
		RenderLayer::CProperties *pProperty = GetQuadLayerProperties(static_cast<RenderLayer::EQuadLayers>(i));
		//if (pProperty->IsActive())
		{
			CryVR::Hypereal::SHmdRenderLayerInfo &_data = m_renderLayerInfoArray[i];
			_data.bActive = pProperty->IsActive();
			_data.layerId = pProperty->GetId();
			_data.layerType = pProperty->GetType();
			_data.pose = pProperty->GetPose();
			_data.viewportPosition = Vec2i(0, 0);
			_data.viewportSize = Vec2i(1024, 1024);
		}
	}
}
#endif //defined(INCLUDE_VR_RENDERING)
