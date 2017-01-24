// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/VR/IHMDDevice.h>

namespace CryVR
{
namespace Hypereal
{

struct TextureDesc
{
	uint32 width;
	uint32 height;
	uint32 format;
};

enum ERenderAPI
{
	eRenderAPI_DirectX = 0,
	eRenderAPI_OpenGL  = 1,
};

enum ERenderColorSpace
{
	eRenderColorSpace_Auto   = 0,
	eRenderColorSpace_Gamma  = 1,
	eRenderColorSpace_Linear = 2,
};

struct IHyperealDevice : public IHmdDevice
{
public:
	virtual void OnSetupEyeTargets(ERenderAPI api, ERenderColorSpace colorSpace, void* leftEyeHandle, void* rightEyeHandle) = 0;
	virtual void OnSetupOverlay(int id, void* overlayTextureHandle) = 0;
	virtual void OnDeleteOverlay(int id) = 0;
	virtual void SubmitOverlay(int id) = 0;
	virtual void SubmitFrame() = 0;
	virtual void GetRenderTargetSize(uint& w, uint& h) = 0;
	virtual void GetMirrorImageView(EEyeType eye, void* resource, void** mirrorTextureView) = 0;
	virtual void CopyMirrorImage(void* pDstResource, uint nWidth, uint nHeight) = 0;

	virtual void CreateGraphicsContext(void* graphicsDevice) = 0;
	virtual void ReleaseGraphicsContext() = 0;
protected:
	virtual ~IHyperealDevice() {}
};
}      // namespace Hypereal
}      // namespace CryVR
