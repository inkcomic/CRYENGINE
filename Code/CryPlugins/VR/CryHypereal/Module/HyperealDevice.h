#pragma once

#include <CrySystem/VR/IHMDDevice.h>
#include <CrySystem/VR/IHMDManager.h>

#include "../Interface/IHmdHyperealDevice.h"
#include "HyperealController.h"

#include <Cry3DEngine/IIndexedMesh.h>
#include <CryRenderer/IStereoRenderer.h>

struct IConsoleCmdArgs;
struct IRenderer;

namespace CryVR
{
namespace Hypereal
{
class Controller;
class Device : public IHyperealDevice, public IHmdEventListener, public ISystemEventListener
{
public:
	// IHmdDevice
	virtual void                    AddRef() override;
	virtual void                    Release() override;

	virtual EHmdClass               GetClass() const override                         { return eHmdClass_Hypereal; }
	virtual void                    GetDeviceInfo(HmdDeviceInfo& info) const override { info = m_devInfo; }

	virtual void                    GetCameraSetupInfo(float& fov, float& aspectRatioFactor) const override;
	virtual void                    GetAsymmetricCameraSetupInfo(int nEye, float& fov, float& aspectRatio, float& asymH, float& asymV, float& eyeDist) const;
	virtual void                    UpdateInternal(EInternalUpdate type) override;
	virtual void                    RecenterPose() override;
	virtual void                    UpdateTrackingState(EVRComponent type) override;
	virtual const HmdTrackingState& GetNativeTrackingState() const override;
	virtual const HmdTrackingState& GetLocalTrackingState() const override;
	virtual Quad                    GetPlayArea() const override;
	virtual Vec2                    GetPlayAreaSize() const override;
	virtual const IHmdController*   GetController() const override      { return &m_controller; }
	virtual const EHmdSocialScreen  GetSocialScreenType(bool* pKeepAspect = nullptr) const override;
	virtual int                     GetControllerCount() const override { __debugbreak(); return 2; /* Hypereal_TODO */ }
	virtual void                    GetPreferredRenderResolution(unsigned int& width, unsigned int& height) override;
	virtual void                    DisableHMDTracking(bool disable) override;
	// ~IHmdDevice

	// IHyperealDevice
	virtual void SubmitOverlay(int id);
	virtual void SubmitFrame(const CryVR::Hypereal::SHmdSubmitFrameData& submitData);
	virtual void OnSetupEyeTargets(ERenderAPI api, ERenderColorSpace colorSpace, void* leftEyeHandle, void* rightEyeHandle);
	virtual void OnSetupOverlay(int id,void* overlayTextureHandle);
	virtual void OnDeleteOverlay(int id);
	virtual void GetRenderTargetSize(uint& w, uint& h);
	virtual void GetMirrorImageView(EEyeType eye, void* resource, void** mirrorTextureView) override;
	virtual void CopyMirrorImage(void* pDstResource, uint nWidth, uint nHeight)override;
	// ~IHyperealDevice

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
	// ~ISystemEventListener

	// IHmdEventListener
	virtual void OnRecentered() override;
	// ~IHmdEventListener

public:
	int GetRefCount() const { return m_refCount; }

public:
	static Device* CreateInstance();
	void           SetupRenderModels();
	void           CaptureInputFocus(bool capture);
	bool           HasInputFocus() { return m_hasInputFocus; }

private:
	Device();
	virtual ~Device();

	void                   CreateDevice();
	void                   PrintHmdInfo();
	void                   DebugDraw(float& xPosLabel, float& yPosLabel) const;

 	const char*            GetTrackedDeviceCharPointer(int nProperty);
 	inline void            CopyPoseState(HmdPoseState& world, HmdPoseState& hmd, HyTrackingState& source);
private:
	
	struct SOverlay
	{
		bool					visible;
		bool					submitted;
		HyViewLayer*			layerHandle;
		void*					overlayTexture;
		HyTextureDesc			textureDesc;
	};

	typedef std::map<uint, SOverlay> MapOverlayer;
	MapOverlayer			m_mapOverlayers;
	SOverlay                m_overlays[RenderLayer::eQuadLayers_Total];
	// General device fields:
	bool                    m_bLoadingScreenActive;
	volatile int            m_refCount;
	int                     m_lastFrameID_UpdateTrackingState; // we could remove this. at some point we may want to sample more than once the tracking state per frame.
	HmdDeviceInfo           m_devInfo;
	EHmdSocialScreen        m_defaultSocialScreenBehavior;
	// Tracking related:
	enum EDevice
	{
		Hmd,
		Left_Controller,
		Right_Controller,
		Total_Count
	};
	HyTrackingState			m_rTrackedDevicePose[EDevice::Total_Count];
	HmdTrackingState        m_nativeStates[EDevice::Total_Count];
	HmdTrackingState        m_localStates[EDevice::Total_Count];
	HmdTrackingState        m_nativeEyePoseStates;
	HmdTrackingState        m_localEyePoseStates;

	HmdTrackingState        m_disabledTrackingState;

	Quat					m_qBaseOrientation;
	Vec3					m_vBaseOffset;
	float					m_fMeterToWorldScale;
	bool					m_bPosTrackingEnable;
	bool					m_bResetOrientationKeepPitchAndRoll;

	// Controller related:
	Controller              m_controller;
	bool                    m_hasInputFocus;
	bool                    m_hmdTrackingDisabled;
	float                   m_hmdQuadDistance;
	float                   m_hmdQuadWidth;
	int                     m_hmdQuadAbsolute;

	ICVar*                  m_pHmdInfoCVar;
	ICVar*                  m_pHmdSocialScreenKeepAspectCVar;
	ICVar*                  m_pHmdSocialScreenCVar;
	ICVar*                  m_pTrackingOriginCVar;

	//////////////////////////////////////////////////////////////////////////
	//HVR device member
	struct DeviceInfo
	{
		int64 DeviceResolutionX;
		int64 DeviceResolutionY;
		HyFov Fov[HY_EYE_MAX];
	};



	HyDevice *m_pVrDevice;
	DeviceInfo m_VrDeviceInfo;
	HyGraphicsContext *m_pVrGraphicsCxt;
	HyGraphicsContextDesc m_VrGraphicsCxtDesc;
	HyVec2 *m_pPlayAreaVertices;
	int64 m_nPlayAreaVertexCount;
	bool m_bPlayAreaValid;
	HyFov m_eyeFovSym;
	float m_fPixelDensity;
	bool m_bVRInitialized;
	bool m_bVRSystemValid;
	bool m_bIsQuitting;
	HyTextureDesc m_RTDesc[2];
	float m_fInterpupillaryDistance;

	HyPose	m_CurDevicePose[EDevice::Total_Count];
	bool	m_IsDevicePositionTracked[EDevice::Total_Count];
	bool	m_IsDeviceRotationTracked[EDevice::Total_Count];
	//////////////////////////////////////////////////////////////////////////

	//member func
	void RebuildPlayArea();
	float GetDistance(const HyVec2& P, const HyVec2& PA, const HyVec2& PB);
	void ReleaseDevice();
	inline float GetInterpupillaryDistance() const;
	void ResetOrientationAndPosition(float Yaw);
	void ResetOrientation(float Yaw);
	void ResetPosition();
	void CreateLayer(int id, void* overlayTextureHandle, bool bRecrate = false);
	void DestroyLayer(int id);
	void SetShowLayer(int id, bool bShow);
public:
	virtual void CreateGraphicsContext(void* graphicsDevice);
	virtual void ReleaseGraphicsContext();
};
} // namespace Hypereal
} // namespace CryVR
