#include "StdAfx.h"

#include "HyperealDevice.h"
#include "HyperealResources.h"
#include "HyperealController.h"

#include "PluginDll.h"

#include <CrySystem/VR/IHMDManager.h>

#include <CryGame/IGameFramework.h>
#include <CryRenderer/IStereoRenderer.h>
#include <Cry3DEngine/IIndexedMesh.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <CryMath/Cry_Color.h>

#include "..\CryAction\IViewSystem.h"
#include <CryCore/Platform/CryWindows.h>

#define HY_RELEASE(p) {if(p != nullptr) p->Release(); p = nullptr;}

HyFov ComputeSymmetricalFov(const HyFov& fovLeftEye, const HyFov& fovRightEye)
{
	const float stereoDeviceAR = 1.7777f;

	HyFov fovMax;
	fovMax.m_upTan = max(fovLeftEye.m_upTan, fovRightEye.m_upTan);
	fovMax.m_downTan = max(fovLeftEye.m_downTan, fovRightEye.m_downTan);
	fovMax.m_leftTan = max(fovLeftEye.m_leftTan, fovRightEye.m_leftTan);
	fovMax.m_rightTan = max(fovLeftEye.m_rightTan, fovRightEye.m_rightTan);

	const float combinedTanHalfFovHorizontal = max(fovMax.m_leftTan, fovMax.m_rightTan);
	const float combinedTanHalfFovVertical = max(fovMax.m_upTan, fovMax.m_downTan);

	HyFov fovSym;
	fovSym.m_upTan = fovSym.m_downTan = combinedTanHalfFovVertical * 1.f;
	
	fovSym.m_leftTan = fovSym.m_rightTan = fovSym.m_upTan / (2.f / stereoDeviceAR);

	CryLog("[Hypereal] Fov: Up/Down tans [%f] Left/Right tans [%f]", fovSym.m_upTan, fovSym.m_leftTan);
	return fovSym;
}


// 
// // -------------------------------------------------------------------------
// namespace vr
// {
// // -------------------------------------------------------------------------
// static void SetIdentity(HmdMatrix34_t& in)
// {
// 	memset(in.m, 0, sizeof(HmdMatrix34_t));
// 	in.m[0][0] = 1;
// 	in.m[1][1] = 1;
// 	in.m[2][2] = 1;
// }
// 
// // -------------------------------------------------------------------------
// static void SetZero(HmdVector3_t& in)
// {
// 	memset(in.v, 0, sizeof(HmdVector3_t));
// }
// 
// static Matrix34 RawConvert(HmdMatrix34_t in)
// {
// 	return Matrix34(in.m[0][0], in.m[0][1], in.m[0][2], in.m[0][3],
// 	                in.m[1][0], in.m[1][1], in.m[1][2], in.m[1][3],
// 	                in.m[2][0], in.m[2][1], in.m[2][2], in.m[2][3]);
// }
// 
// static Matrix44 RawConvert(HmdMatrix44_t in)
// {
// 	return Matrix44(in.m[0][0], in.m[0][1], in.m[0][2], in.m[0][3],
// 	                in.m[1][0], in.m[1][1], in.m[1][2], in.m[1][3],
// 	                in.m[2][0], in.m[2][1], in.m[2][2], in.m[2][3],
// 	                in.m[3][0], in.m[3][1], in.m[3][2], in.m[3][3]);
// }
// 
// static HmdMatrix34_t RawConvert(Matrix34 in)
// {
// 	HmdMatrix34_t out;
// 	out.m[0][0] = in.m00;
// 	out.m[0][1] = in.m01;
// 	out.m[0][2] = in.m02;
// 	out.m[0][3] = in.m03;
// 	out.m[1][0] = in.m10;
// 	out.m[1][1] = in.m11;
// 	out.m[1][2] = in.m12;
// 	out.m[1][3] = in.m13;
// 	out.m[2][0] = in.m20;
// 	out.m[2][1] = in.m21;
// 	out.m[2][2] = in.m22;
// 	out.m[2][3] = in.m23;
// 	return out;
// }
// 
// static HmdMatrix44_t RawConvert(Matrix44 in)
// {
// 	HmdMatrix44_t out;
// 	out.m[0][0] = in.m00;
// 	out.m[0][1] = in.m01;
// 	out.m[0][2] = in.m02;
// 	out.m[0][3] = in.m03;
// 	out.m[1][0] = in.m10;
// 	out.m[1][1] = in.m11;
// 	out.m[1][2] = in.m12;
// 	out.m[1][3] = in.m13;
// 	out.m[2][0] = in.m20;
// 	out.m[2][1] = in.m21;
// 	out.m[2][2] = in.m22;
// 	out.m[2][3] = in.m23;
// 	out.m[3][0] = in.m30;
// 	out.m[3][1] = in.m31;
// 	out.m[3][2] = in.m32;
// 	out.m[3][3] = in.m33;
// 	return out;
// }
// }

namespace CryVR {
namespace Hypereal {
// -------------------------------------------------------------------------
Device::RenderModel::RenderModel(/*vr::IVRRenderModels* renderModels, string name*/)
// 	: m_renderModels(renderModels)
// 	, m_name(name)
// 	, m_model(nullptr)
// 	, m_texture(nullptr)
// 	, m_modelState(eRMS_Loading)
// 	, m_textureState(eRMS_Loading)
{

}

Device::RenderModel::~RenderModel()
{
	// Note: we currently do not use the render models, therefore we also do not set them up for rendering!
	/*vertices.clear();
	   normals.clear();
	   uvs.clear();
	   indices.clear();*/

// 	m_model = nullptr; // Object is managed by Hypereal runtime. Since it is possible, that the rendermodel is being accessed by something else, we DO NOT want to delete it
// 	m_modelState = eRMS_Failed;
// 	m_texture = nullptr; // Object is managed by Hypereal runtime. Since it is possible, that the texture is being accessed by something else, we DO NOT want to delete it
// 	m_textureState = eRMS_Failed;
// 
// 	m_renderModels = nullptr;
}

void Device::RenderModel::Update()
{
// 	if (m_modelState == eRMS_Loading) // check model loading progress
// 	{
// 		vr::EVRRenderModelError result = m_renderModels->LoadRenderModel_Async(m_name.c_str(), &m_model);
// 		if (result == vr::VRRenderModelError_Loading)
// 		{
// 			// still loading
// 		}
// 		else if (result == vr::VRRenderModelError_None)
// 		{
// 			m_modelState = eRMS_Loaded;
// 
// 			// Note: we currently do not use the render models, therefore we also do not set them up for rendering!
// 			/*vertices.reserve(m_model->unVertexCount);
// 			   normals.reserve(m_model->unVertexCount);
// 			   uvs.reserve(m_model->unVertexCount);
// 			   indices.reserve(m_model->unVertexCount);
// 
// 			   for (int i = 0; i < m_model->unVertexCount; i++)
// 			   {
// 			   const vr::RenderModel_Vertex_t vrVert = m_model->rVertexData[i];
// 			   vertices.Add(Vec3(vrVert.vPosition.v[0], -vrVert.vPosition.v[2], vrVert.vPosition.v[1]));
// 			   normals.Add(Vec3(vrVert.vNormal.v[0], -vrVert.vNormal.v[2], vrVert.vNormal.v[1]));
// 			   uvs.Add(Vec2(vrVert.rfTextureCoord[0], vrVert.rfTextureCoord[1]));
// 			   indices.Add(m_model->rIndexData[i]);
// 			   }*/
// 		}
// 		else
// 		{
// 			m_modelState = eRMS_Loaded;
// 		}
// 	}
// 
// 	if (m_modelState == eRMS_Loaded && m_textureState == eRMS_Loading) // check texture loading progress
// 	{
// 		vr::EVRRenderModelError result = m_renderModels->LoadTexture_Async(m_model->diffuseTextureId, &m_texture);
// 		if (result == vr::VRRenderModelError_Loading)
// 		{
// 			// still loading
// 		}
// 		else if (result == vr::VRRenderModelError_None)
// 		{
// 			m_textureState = eRMS_Loaded;
// 
// 			// TODO: Setup engine texture
// 
// 			gEnv->pLog->Log("[HMD][Hypereal] Device render model loaded: %s", m_name.c_str());
// 		}
// 		else
// 		{
// 			m_textureState = eRMS_Failed;
// 		}
// 	}
}

static ICVar* pParallax = nullptr;
// -------------------------------------------------------------------------
Device::Device()
	: m_refCount(1)     //OpenVRResources.cpp assumes refcount is 1 on allocation
	, m_controller()
	, m_lastFrameID_UpdateTrackingState(-1)
	, m_devInfo()
	, m_hasInputFocus(false)
	, m_bLoadingScreenActive(false)
	, m_hmdTrackingDisabled(false)
	, m_hmdQuadDistance(CPlugin_Hypereal::s_hmd_quad_distance)
	, m_hmdQuadWidth(CPlugin_Hypereal::s_hmd_quad_width)
	, m_hmdQuadAbsolute(CPlugin_Hypereal::s_hmd_quad_absolute)
	, VrDevice(nullptr)
	, VrGraphicsCxt(nullptr)
	, PlayAreaVertices(nullptr)
	, bPlayAreaValid(false)
	, PixelDensity(1.0f)
	, bVRInitialized(nullptr)
	, bVRSystemValid(nullptr)
	, bIsQuitting(false)
{
	m_pHmdInfoCVar = gEnv->pConsole->GetCVar("hmd_info");
	m_pHmdSocialScreenKeepAspectCVar = gEnv->pConsole->GetCVar("hmd_social_screen_keep_aspect");
	m_pHmdSocialScreenCVar = gEnv->pConsole->GetCVar("hmd_social_screen");
	m_pTrackingOriginCVar = gEnv->pConsole->GetCVar("hmd_tracking_origin");

	CreateDevice();

	gEnv->pSystem->GetHmdManager()->AddEventListener(this);

	pParallax = gEnv->pConsole->GetCVar("sys_flash_stereo_maxparallax");

	// 	memset(m_rTrackedDevicePose, 0, sizeof(vr::TrackedDevicePose_t) * vr::k_unMaxTrackedDeviceCount);
	// 	for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
	// 	{
	// 		m_deviceModels[i] = nullptr;
	// 		vr::SetIdentity(m_rTrackedDevicePose[i].mDeviceToAbsoluteTracking);
	// 	}
	// 	for (int i = 0; i < EEyeType::eEyeType_NumEyes; i++)
	// 		m_eyeTargets[i] = nullptr;
	// 
	if (GetISystem()->GetISystemEventDispatcher())
		GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

	m_controller.Init(VrDevice);

	if(m_controller.IsConnected(EHmdController::eHmdController_Hypereal_1))
		m_controller.OnControllerConnect(HY_SUBDEV_CONTROLLER_LEFT);

	if(m_controller.IsConnected(EHmdController::eHmdController_Hypereal_2))
		m_controller.OnControllerConnect(HY_SUBDEV_CONTROLLER_RIGHT);
	
		
	for (int id = 0; id < RenderLayer::eQuadLayers_Total; id++)
	{
		memset(&m_overlays[id], 0, sizeof(m_overlays[id]));
	}
	
}

// -------------------------------------------------------------------------
Device::~Device()
{
	gEnv->pSystem->GetHmdManager()->RemoveEventListener(this);

	if (GetISystem()->GetISystemEventDispatcher())
		GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
}
// 
// // -------------------------------------------------------------------------
// string Device::GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::ETrackedDeviceProperty prop, vr::ETrackedPropertyError* peError)
// {
// 	uint32_t unRequiredBufferLen = m_system->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
// 	if (unRequiredBufferLen == 0)
// 		return "";
// 
// 	char* pchBuffer = new char[unRequiredBufferLen];
// 	unRequiredBufferLen = m_system->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
// 	string result = string(pchBuffer, unRequiredBufferLen);
// 	delete[] pchBuffer;
// 	return result;
// }
// 
// -------------------------------------------------------------------------
const char* Device::GetTrackedDeviceCharPointer(int nProperty)
{
	int realStrLen = 512;
	//VrDevice->GetStringValue(HY_PROPERTY_DEVICE_MANUFACTURER_STRING, 0, 0, &realStrLen);

	//if (realStrLen == 0)
	//	return nullptr;

	char* pBuffer = new char[realStrLen];
	VrDevice->GetStringValue(HY_PROPERTY_DEVICE_MANUFACTURER_STRING, pBuffer, realStrLen, &realStrLen);
	return const_cast<char*>(pBuffer);
}
// 
// // -------------------------------------------------------------------------
// Matrix34 Device::BuildMatrix(const vr::HmdMatrix34_t& in)
// {
// 	return Matrix34(in.m[0][0], in.m[0][1], in.m[0][2], in.m[0][3],
// 	                in.m[1][0], in.m[1][1], in.m[1][2], in.m[1][3],
// 	                in.m[2][0], in.m[2][1], in.m[2][2], in.m[2][3]);
// }
// 
// // -------------------------------------------------------------------------
// Matrix44 Device::BuildMatrix(const vr::HmdMatrix44_t& in)
// {
// 	return Matrix44(in.m[0][0], in.m[0][1], in.m[0][2], in.m[0][3],
// 	                in.m[1][0], in.m[1][1], in.m[1][2], in.m[1][3],
// 	                in.m[2][0], in.m[2][1], in.m[2][2], in.m[2][3],
// 	                in.m[3][0], in.m[3][1], in.m[3][2], in.m[3][3]);
// }
// 
// -------------------------------------------------------------------------
Quat Device::HmdQuatToWorldQuat(const Quat& quat)
{
	Matrix33 m33(quat);
	Vec3 column1 = -quat.GetColumn2();
	m33.SetColumn2(m33.GetColumn1());
	m33.SetColumn1(column1);
	return Quat::CreateRotationX(gf_PI * 0.5f) * Quat(m33);
}
 
 // -------------------------------------------------------------------------
 Vec3 Device::HmdVec3ToWorldVec3(const Vec3& vec)
 {
 	return Vec3(vec.x, -vec.z, vec.y);
 }
 
 inline Quat HYQuatToQuat(const HyQuat& q) {
	 return Quat(q.w,q.x,q.y,q.z);
 }
 inline Vec3 HYVec3ToVec3(const HyVec3& v) {
	 return Vec3(v.z,v.y,v.z);
 }
 inline HyQuat QuatToHYQuat(const Quat &q) {
	 HyQuat hyQuat;
	 hyQuat.w = q.w;
	 hyQuat.x = q.v.x;
	 hyQuat.y = q.v.y;
	 hyQuat.z = q.v.z;
	 return hyQuat;
 }
 inline HyVec3 Vec3ToHYVec3(const Vec3& v) {
	 HyVec3 hyVec3;
	 hyVec3.x = v.x;
	 hyVec3.y = v.y;
	 hyVec3.z = v.z;
	 return hyVec3;
 }
// -------------------------------------------------------------------------
void Device::CopyPoseState(HmdPoseState& world, HmdPoseState& hmd, HyTrackingState& src)
{
	Quat srcQuat = HYQuatToQuat(src.m_pose.m_rotation);
	Vec3 srcPos = HYVec3ToVec3(src.m_pose.m_position);
	Vec3 srcAngVel = HYVec3ToVec3(src.m_angularVelocity);
	Vec3 srcAngAcc = HYVec3ToVec3(src.m_angularAcceleration);
	Vec3 srcLinVel = HYVec3ToVec3(src.m_linearVelocity);
	Vec3 srcLinAcc = HYVec3ToVec3(src.m_linearAcceleration);

	hmd.orientation = srcQuat;
	hmd.position = srcPos;

	hmd.linearVelocity = srcLinVel;
	hmd.angularVelocity = srcAngVel;
 
	world.position = HmdVec3ToWorldVec3(hmd.position);
	world.orientation = HmdQuatToWorldQuat(hmd.orientation);
	world.linearVelocity = HmdVec3ToWorldVec3(hmd.linearVelocity);
	world.angularVelocity = HmdVec3ToWorldVec3(hmd.angularVelocity);
}

// -------------------------------------------------------------------------
void Device::SetupRenderModels()
{
// 	gEnv->pLog->Log("[HMD][Hypereal] SetupRenderModels");
// 	for (int dev = vr::k_unTrackedDeviceIndex_Hmd + 1; dev < vr::k_unMaxTrackedDeviceCount; dev++)
// 	{
// 		if (!m_system->IsTrackedDeviceConnected(dev))
// 			continue;
// 
// 		LoadDeviceRenderModel(dev);
// 	}
}

// -------------------------------------------------------------------------
void Device::CaptureInputFocus(bool capture)
{
// 	if (capture == m_hasInputFocus)
// 		return;
// 
// 	if (capture)
// 	{
// 		m_hasInputFocus = m_system->CaptureInputFocus();
// 	}
// 	else
// 	{
// 		m_system->ReleaseInputFocus();
// 		m_hasInputFocus = false;
// 	}
}
// 
// // -------------------------------------------------------------------------
// void Device::LoadDeviceRenderModel(int deviceIndex)
// {
// 	if (!m_renderModels)
// 		return;
// 
// 	vr::ETrackedPropertyError eError = vr::ETrackedPropertyError::TrackedProp_Success;
// 	string strModel = GetTrackedDeviceString(deviceIndex, vr::ETrackedDeviceProperty::Prop_RenderModelName_String, &eError);
// 	if (eError != vr::ETrackedPropertyError::TrackedProp_Success)
// 	{
// 		gEnv->pLog->Log("[HMD][Hypereal] Could not load device render model: %s", m_system->GetPropErrorNameFromEnum(eError));
// 		return;
// 	}
// 
// 	m_deviceModels[deviceIndex] = new RenderModel(m_renderModels, strModel.c_str());
// 	gEnv->pLog->Log("[HMD][Hypereal] Device render model loading: %s", strModel.c_str());
// 
// }
// 
// // -------------------------------------------------------------------------
// void Device::DumpDeviceRenderModel(int deviceIndex)
// {
// 	SAFE_DELETE(m_deviceModels[deviceIndex]);
// }

// -------------------------------------------------------------------------
void Device::AddRef()
{
	CryInterlockedIncrement(&m_refCount);
}

// -------------------------------------------------------------------------
void Device::Release()
{
	long refCount = CryInterlockedDecrement(&m_refCount);
#if !defined(_RELEASE)
	IF (refCount < 0, 0)
		__debugbreak();
#endif
	IF (refCount == 0, 0)
	{
		delete this;
	}
}

// -------------------------------------------------------------------------
void Device::GetPreferredRenderResolution(unsigned int& width, unsigned int& height)
{
	GetRenderTargetSize(width, height);
}

// -------------------------------------------------------------------------
void Device::DisableHMDTracking(bool disable)
{
	m_hmdTrackingDisabled = disable;
}

// -------------------------------------------------------------------------
Device* Device::CreateInstance()
{
	return new Device();
}

// -------------------------------------------------------------------------
void Device::GetCameraSetupInfo(float& fov, float& aspectRatioFactor) const
{
// 	float fNear = gEnv->pRenderer->GetCamera().GetNearPlane();
// 	float fFar = gEnv->pRenderer->GetCamera().GetFarPlane();
// 
// 	HyMat4 proj;
// 	VrGraphicsCxt->GetProjectionMatrix(VrDeviceInfo.Fov[HY_EYE_LEFT], fNear, fFar, true, proj);
// 	fov = 2.0f * atan(1.0f / proj.m[1][1]);
// 	aspectRatioFactor = 2.0f;
	fov = m_devInfo.fovV;
	aspectRatioFactor = 2.0f * eyeFovSym.m_leftTan;
// 	vr::HmdMatrix44_t proj = m_system->GetProjectionMatrix(vr::EVREye::Eye_Left, fNear, fFar, vr::API_DirectX);
// 
// 	fov = 2.0f * atan(1.0f / proj.m[1][1]);
// 	aspectRatioFactor = 2.0f;
}

// -------------------------------------------------------------------------
void Device::GetAsymmetricCameraSetupInfo(int nEye, float& fov, float& aspectRatio, float& asymH, float& asymV, float& eyeDist) const
{
// 	float fLeft, fRight, fTop, fBottom;
// 	HyFov hyFov = VrDeviceInfo.Fov[(HyEye)nEye];
// 	fLeft = hyFov.m_leftTan;
// 	fRight = hyFov.m_rightTan;
// 	fTop = hyFov.m_upTan;
// 	fBottom = hyFov.m_downTan;
// 
// 
// 	fov = 2.0f * atan((fBottom + fTop) / 2.0f);
// 	aspectRatio = (fRight + fLeft) / (fBottom + fTop);
// 	asymH = (fRight + fLeft) / 2;
// 	asymV = (fBottom + fTop) / 2;

	float fNear = gEnv->pRenderer->GetCamera().GetNearPlane();
	float fFar = gEnv->pRenderer->GetCamera().GetFarPlane();
	HyMat4 proj;
	VrGraphicsCxt->GetProjectionMatrix(VrDeviceInfo.Fov[nEye], fNear, fFar, true, proj);
	fov = 2.0f * atan(1.0f / proj.m[1][1]);
	aspectRatio = proj.m[1][1] / proj.m[0][0];
	asymH = proj.m[0][2] / proj.m[1][1] * aspectRatio;
	asymV = proj.m[1][2] / proj.m[1][1];


	if (VrDevice)
		VrDevice->GetFloatValue(HY_PROPERTY_IPD_FLOAT, eyeDist);
	else
		eyeDist = 1.6f;

//	float fLeft, fRight, fTop, fBottom;
//	m_system->GetProjectionRaw((vr::EVREye)nEye, &fLeft, &fRight, &fTop, &fBottom);
// 	fov = 2.0f * atan((fBottom - fTop) / 2.0f);
// 	aspectRatio = (fRight - fLeft) / (fBottom - fTop);
// 	asymH = (fRight + fLeft) / 2;
// 	asymV = (fBottom + fTop) / 2;

//	eyeDist = m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_UserIpdMeters_Float, nullptr);
}

// -------------------------------------------------------------------------
void Device::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN:
		m_bLoadingScreenActive = true;
		break;

	// Intentional fall through
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
	case ESYSTEM_EVENT_LEVEL_LOAD_ERROR:
		m_bLoadingScreenActive = false;
		m_controller.ClearState();
		break;

	case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
		m_controller.ClearState();
		break;

	default:
		break;
	}
}

// -------------------------------------------------------------------------
void Device::UpdateTrackingState(EVRComponent type)
{
	IRenderer* pRenderer = gEnv->pRenderer;

	const int frameID = pRenderer->GetFrameID(false);

#if !defined(_RELEASE)
	if (!gEnv->IsEditor())// we currently assume one system update per frame rendered, which is not always the case in editor (i.e. no level)
	{
		if (((type & eVRComponent_Hmd) != 0) && (CryGetCurrentThreadId() != gEnv->mMainThreadId) && (m_bLoadingScreenActive == false))
		{
			gEnv->pLog->LogError("[HMD][Hypereal] Device::UpdateTrackingState() should be called from main thread only!");
		}

		if (frameID == m_lastFrameID_UpdateTrackingState)
		{
			gEnv->pLog->LogError("[HMD][Hypereal] Device::UpdateTrackingState() should be called only once per frame!");
		}
	}
	m_lastFrameID_UpdateTrackingState = frameID;
#endif


	if (VrDevice)
	{
		VrDevice->ConfigureTrackingOrigin(m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor == 1 ? HY_TRACKING_ORIGIN_FLOOR : HY_TRACKING_ORIGIN_EYE);

		static const HySubDevice Devices[EDevice::Total_Count] =
		{ HY_SUBDEV_HMD, HY_SUBDEV_CONTROLLER_LEFT, HY_SUBDEV_CONTROLLER_RIGHT };

		HyTrackingState trackingState;
		for (uint32_t i = 0; i < EDevice::Total_Count; i++)
		{
			HyResult r = VrDevice->GetTrackingState(Devices[i], frameID, trackingState);
			if (hySucceeded(r))
			{
				m_CurEyePoses[i] = trackingState.m_pose;

				m_rTrackedDevicePose[i] = trackingState;
				m_IsDevicePositionTracked[i] = ((HY_TRACKING_POSITION_TRACKED & trackingState.m_flags) != 0);
				m_IsDeviceRotationTracked[i] = ((HY_TRACKING_ROTATION_TRACKED & trackingState.m_flags) != 0);


				m_localStates[i].statusFlags = m_nativeStates[i].statusFlags = ((m_IsDeviceRotationTracked[EDevice::Hmd]) ? eHmdStatus_OrientationTracked : 0) |
					((m_IsDevicePositionTracked[EDevice::Hmd]) ? eHmdStatus_PositionTracked : 0);


				if (m_IsDevicePositionTracked[i]||m_IsDeviceRotationTracked[i])
				{
					//need retransform
					float* ipdptr = (InterpupillaryDistance > 0.01f ? &InterpupillaryDistance : nullptr);
					VrGraphicsCxt->GetEyePoses(CurDevicePose[EHyMotionDevice::Hmd], ipdptr, CurrentRenderPoses);
					FQuat eyeOrientation;
					FVector eyePosition;
					PoseToOrientationAndPosition(CurrentRenderPoses[(InView.StereoPass == eSSP_LEFT_EYE) ? HY_EYE_LEFT : HY_EYE_RIGHT], eyeOrientation, eyePosition);



					CopyPoseState(m_localStates[i].pose, m_nativeStates[i].pose, m_rTrackedDevicePose[i]);
				}

			}
			else
			{
				m_IsDevicePositionTracked[i] = false;
				m_IsDeviceRotationTracked[i] = false;
			}

			if(i!= Hmd)
			{
				HyResult res;
				HyInputState controllerState;

				HySubDevice sid = static_cast<HySubDevice>(i);
				res = VrDevice->GetControllerInputState(sid, controllerState);
				if (res)
					m_controller.Update(sid, m_nativeStates[i], m_localStates[i], controllerState);
			}
		}

	}


// 	vr::Compositor_FrameTiming ft;
// 	ft.m_nSize = sizeof(vr::Compositor_FrameTiming);
// 	m_compositor->GetFrameTiming(&ft, 0);
// 
// 	float fSecondsSinceLastVsync = ft.m_flPresentCallCpuMs / 1000.0f;
// 	float rFreq = m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_DisplayFrequency_Float);
// 	if (rFreq < 1 || !NumberValid(rFreq)) // in case we can't get a proper refresh rate, we just assume 90 fps, to avoid crashes
// 		rFreq = 90.0f;
// 	float fFrameDuration = 1.0f / rFreq;
// 	float fSecondsUntilPhotons = 3.0f * fFrameDuration - fSecondsSinceLastVsync + m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_SecondsFromVsyncToPhotons_Float);
// 	m_system->GetDeviceToAbsoluteTrackingPose(m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor ? vr::ETrackingUniverseOrigin::TrackingUniverseStanding : vr::ETrackingUniverseOrigin::TrackingUniverseSeated, fSecondsUntilPhotons, m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount);
// 
// 	IView* pView = gEnv->pGameFramework->GetIViewSystem()->GetActiveView();
// 	const SViewParams* params = pView ? pView->GetCurrentParams() : nullptr;
// 	Vec3 pos = params ? params->position : Vec3(0, 0, 0);
// 	Quat rot = params ? params->rotation : Quat::CreateIdentity();
// 	for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
// 	{
// 		m_localStates[i].statusFlags = m_nativeStates[i].statusFlags = ((m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) ? eHmdStatus_OrientationTracked : 0) |
// 		                                                               ((m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) ? eHmdStatus_PositionTracked : 0) |
// 		                                                               ((m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) ? eHmdStatus_CameraPoseTracked : 0) |
// 		                                                               ((m_system->IsTrackedDeviceConnected(vr::k_unTrackedDeviceIndex_Hmd)) ? eHmdStatus_PositionConnected : 0) |
// 		                                                               ((m_system->IsTrackedDeviceConnected(vr::k_unTrackedDeviceIndex_Hmd)) ? eHmdStatus_HmdConnected : 0);
// 
// 		if (m_rTrackedDevicePose[i].bPoseIsValid && m_rTrackedDevicePose[i].bDeviceIsConnected)
// 		{
// 			CopyPoseState(m_localStates[i].pose, m_nativeStates[i].pose, m_rTrackedDevicePose[i]);
// 
// 			vr::ETrackedDeviceClass devClass = m_system->GetTrackedDeviceClass(i);
// 			switch (devClass)
// 			{
// 			case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
// 				{
// 					vr::VRControllerState_t state;
// 					if (m_system->GetControllerState(i, &state))
// 					{
// 						m_controller.Update(i, m_nativeStates[i], m_localStates[i], state);
// 					}
// 				}
// 				break;
// 			}
// 		}
// 	}
// 
// 	if (m_pHmdInfoCVar->GetIVal() > 0)
// 	{
// 		float xPos = 10.f, yPos = 10.f;
// 		DebugDraw(xPos, yPos);
// 		yPos += 25.f;
// 		m_controller.DebugDraw(xPos, yPos);
// 	}
}

// -------------------------------------------------------------------------
void Device::UpdateInternal(EInternalUpdate type)
{
	if (!VrDevice || !VrGraphicsCxt)
	{
		return;
	}
	const HyMsgHeader *msg;
	while (true)
	{
		VrDevice->RetrieveMsg(&msg);
		if (msg->m_type == HY_MSG_NONE)
			break;
		switch (msg->m_type)
		{
		case HY_MSG_PENDING_QUIT:
			bIsQuitting = true;
			break;
		case HY_MSG_INPUT_FOCUS_CHANGED:
// 			bIsPausing = !(((HyMsgFocusChange*)msg)->m_id == HY_ID_SELF_IN_MSG);
// 			if (bIsPausing)
// 				FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Broadcast();
// 			else
// 				FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Broadcast();
 			break;
		case HY_MSG_VIEW_FOCUS_CHANGED:
// 			bIsVisible = ((HyMsgFocusChange*)msg)->m_id == HY_ID_SELF_IN_MSG;
// 			if (bIsVisible)
// 				FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Broadcast();
// 			else
// 				FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Broadcast();
			break;
		case HY_MSG_SUBDEVICE_STATUS_CHANGED:
		{
			HyMsgSubdeviceChange* pData = ((HyMsgSubdeviceChange*)msg);
			HySubDevice sid = static_cast<HySubDevice>(pData->m_subdevice);
			if (0 != pData->m_value)
				m_controller.OnControllerConnect(sid);
			else
				m_controller.OnControllerDisconnect(sid);
		}
			break;
		default:
			VrDevice->DefaultMsgFunction(msg);
			break;
		}
	}

	if (bIsQuitting)
	{
		//EnableStereo(false);
#if WITH_EDITOR
		if (GIsEditor)
		{
			FSceneViewport* SceneVP = FindSceneViewport();
			if (SceneVP && SceneVP->IsStereoRenderingAllowed())
			{
				TSharedPtr<SWindow> Window = SceneVP->FindWindow();
				Window->RequestDestroyWindow();
			}
		}
		else
#endif	//WITH_EDITOR
		{
			gEnv->pSystem->Quit();
		}
		bIsQuitting = false;
	}

// 	// vr event handling
// 	vr::VREvent_t event;
// 	while (m_system->PollNextEvent(&event, sizeof(vr::VREvent_t)))
// 	{
// 		switch (event.eventType)
// 		{
// 		case vr::VREvent_TrackedDeviceActivated:
// 			{
// 				if (m_refCount)
// 					LoadDeviceRenderModel(event.trackedDeviceIndex);
// 				vr::ETrackedDeviceClass devClass = m_system->GetTrackedDeviceClass(event.trackedDeviceIndex);
// 				switch (devClass)
// 				{
// 				case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
// 					{
// 						gEnv->pLog->Log("[HMD][Hypereal] - Controller connected:", event.trackedDeviceIndex);
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Tracked Device Index: %i", event.trackedDeviceIndex);
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Attached Device Id: %s", GetTrackedDeviceCharPointer(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_AttachedDeviceId_String));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Tracking System Name: %s", GetTrackedDeviceCharPointer(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_TrackingSystemName_String));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Model Number: %s", GetTrackedDeviceCharPointer(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_ModelNumber_String));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Serial Number: %s", GetTrackedDeviceCharPointer(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_SerialNumber_String));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Manufacteurer: %s", GetTrackedDeviceCharPointer(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_ManufacturerName_String));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Tracking Firmware Version: %s", GetTrackedDeviceCharPointer(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_TrackingFirmwareVersion_String));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Hardware Revision: %s", GetTrackedDeviceCharPointer(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_HardwareRevision_String));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Is Wireless: %s", (m_system->GetBoolTrackedDeviceProperty(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_DeviceIsWireless_Bool) ? "True" : "False"));
// 						gEnv->pLog->Log("[HMD][Hypereal] --- Is Charging: %s", (m_system->GetBoolTrackedDeviceProperty(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_DeviceIsCharging_Bool) ? "True" : "False"));
// 						//gEnv->pLog->Log("[HMD][Hypereal] --- Battery: %f%%", (m_system->GetFloatTrackedDeviceProperty(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_DeviceBatteryPercentage_Float) * 100.0f)); <-- was test implementation in Hypereal | Values not available (anymore/yet?)!
// 
// 						m_controller.OnControllerConnect(event.trackedDeviceIndex);
// 					}
// 					break;
// 				}
// 			}
// 			break;
// 		case vr::VREvent_TrackedDeviceDeactivated:
// 			{
// 				if (m_renderModels)
// 					DumpDeviceRenderModel(event.trackedDeviceIndex);
// 
// 				vr::ETrackedDeviceClass devClass = m_system->GetTrackedDeviceClass(event.trackedDeviceIndex);
// 				switch (devClass)
// 				{
// 				case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
// 					{
// 						gEnv->pLog->Log("[HMD][Hypereal] - Controller disconnected: %i", event.trackedDeviceIndex);
// 
// 						m_controller.OnControllerDisconnect(event.trackedDeviceIndex);
// 					}
// 					break;
// 				}
// 			}
// 			break;
// 		}
// 	}
// 
// 	switch (type)
// 	{
// 	case IHmdDevice::eInternalUpdate_DebugInfo:
// 		{
// 			if (m_pHmdInfoCVar->GetIVal() != 0)
// 				if (Device* pDevice = Resources::GetAssociatedDevice())
// 					pDevice->PrintHmdInfo();
// 		}
// 		break;
// 	default:
// 		break;
// 	}
// 
// 	// Render model loading
// 	for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
// 	{
// 		if (!m_deviceModels[i])
// 			continue;
// 
// 		if (!m_deviceModels[i]->IsValid())
// 		{
// 			DumpDeviceRenderModel(i);
// 		}
// 		else if (m_deviceModels[i]->IsLoading())
// 		{
// 			m_deviceModels[i]->Update();
// 		}
// 	}
// 
// 	if (m_hmdQuadDistance != CPlugin_Hypereal::s_hmd_quad_distance || m_hmdQuadAbsolute != CPlugin_Hypereal::s_hmd_quad_absolute)
// 	{
// 		m_hmdQuadAbsolute = CPlugin_Hypereal::s_hmd_quad_absolute;
// 		m_hmdQuadDistance = CPlugin_Hypereal::s_hmd_quad_distance;
// 		for (int id = 0; id < RenderLayer::eQuadLayers_Total; id++)
// 		{
// 			m_overlays[id].pos = vr::RawConvert(Matrix34::CreateTranslationMat(Vec3(0, 0, -m_hmdQuadDistance)));
// 			if (m_hmdQuadAbsolute)
// 				m_overlay->SetOverlayTransformAbsolute(m_overlays[id].handle, m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor == 1 ? vr::ETrackingUniverseOrigin::TrackingUniverseStanding : vr::ETrackingUniverseOrigin::TrackingUniverseSeated, &(m_overlays[id].pos));
// 			else
// 				m_overlay->SetOverlayTransformTrackedDeviceRelative(m_overlays[id].handle, vr::k_unTrackedDeviceIndex_Hmd, &(m_overlays[id].pos));
// 		}
// 	}
// 	if (m_hmdQuadWidth != CPlugin_Hypereal::s_hmd_quad_width)
// 	{
// 		m_hmdQuadWidth = CPlugin_Hypereal::s_hmd_quad_width;
// 		for (int id = 0; id < RenderLayer::eQuadLayers_Total; id++)
// 			m_overlay->SetOverlayWidthInMeters(m_overlays[id].handle, m_hmdQuadWidth);
// 	}
}

// -------------------------------------------------------------------------
void Device::CreateDevice()
{
	for (int i = 0; i < 2; ++i)
	{
		RTDesc[i].m_uvSize = HyVec2{ 1.f, 1.f };
		RTDesc[i].m_uvOffset = HyVec2{ 0.f, 0.f };
	}

	HyResult startResult = HyStartup();
	bVRInitialized = hySucceeded(startResult);
	if (!bVRInitialized)
	{
		gEnv->pLog->Log("[HMD][Hypereal] HyperealVR Failed to Startup.");
		return;
	}

	gEnv->pLog->Log("[HMD][Hypereal] HyperealVR Startup sucessfully.");
	
	bPlayAreaValid = false;
	PlayAreaVertexCount = 0;

	
	HyResult hr = HyCreateInterface(sch_HyDevice_Version, 0, (void**)&VrDevice);

	if (!hySucceeded(hr))
	{
		gEnv->pLog->Log("[HMD][Hypereal] HyCreateInterface failed.");
		return ;
	}

	

	memset(&VrDeviceInfo,0, sizeof(DeviceInfo));

	VrDevice->GetIntValue(HY_PROPERTY_DEVICE_RESOLUTION_X_INT, VrDeviceInfo.DeviceResolutionX);
	VrDevice->GetIntValue(HY_PROPERTY_DEVICE_RESOLUTION_Y_INT, VrDeviceInfo.DeviceResolutionY);
	VrDevice->GetFloatArray(HY_PROPERTY_DEVICE_LEFT_EYE_FOV_FLOAT4_ARRAY, VrDeviceInfo.Fov[HY_EYE_LEFT].val, 4);
	VrDevice->GetFloatArray(HY_PROPERTY_DEVICE_RIGHT_EYE_FOV_FLOAT4_ARRAY, VrDeviceInfo.Fov[HY_EYE_RIGHT].val, 4);



	eyeFovSym = ComputeSymmetricalFov( VrDeviceInfo.Fov[HY_EYE_LEFT], VrDeviceInfo.Fov[HY_EYE_RIGHT]);
	//set for  generic HMD device
	m_devInfo.screenWidth = (uint)VrDeviceInfo.DeviceResolutionX;
	m_devInfo.screenHeight = (uint)VrDeviceInfo.DeviceResolutionY;
	
	m_devInfo.manufacturer = GetTrackedDeviceCharPointer(HY_PROPERTY_DEVICE_MANUFACTURER_STRING);
	m_devInfo.productName = GetTrackedDeviceCharPointer(HY_PROPERTY_DEVICE_PRODUCT_NAME_STRING);
	m_devInfo.fovH = 2.0f * atanf(eyeFovSym.m_leftTan);
	m_devInfo.fovV = 2.0f * atanf(eyeFovSym.m_upTan);


	bool isConnected = false;
	hr = VrDevice->GetBoolValue(HY_PROPERTY_HMD_CONNECTED_BOOL, isConnected);
	if (hySucceeded(hr) && isConnected)
	{
	//	CustomPresent = new FHyperealCustomPresent(nullptr, this, VrGraphicsCxt);
		
		VrDevice->ConfigureTrackingOrigin(m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor == 1 ? HY_TRACKING_ORIGIN_FLOOR : HY_TRACKING_ORIGIN_EYE);

		RebuildPlayArea();
		gEnv->pLog->Log("[HMD][Hypereal] EnableStereo successfully.");
	}
	else
	{
		gEnv->pLog->Log("[HMD][Hypereal] HyperealVR HMD is Disconnected.");
	}

	 #if CRY_PLATFORM_WINDOWS
	 	// the following is (hopefully just) a (temporary) hack to shift focus back to the CryEngine window, after (potentially) spawning the SteamVR Compositor
	 	if (!gEnv->IsEditor())
	 	{
	 		LockSetForegroundWindow(LSFW_UNLOCK);
	 		SetForegroundWindow((HWND)gEnv->pSystem->GetHWND());
	 	}
	 #endif
// 	vr::EVRInitError eError = vr::EVRInitError::VRInitError_None;
// 	m_compositor = (vr::IVRCompositor*)vr::VR_GetGenericInterface(vr::IVRCompositor_Version, &eError);
// 	if (eError != vr::EVRInitError::VRInitError_None)
// 	{
// 		m_compositor = nullptr;
// 		gEnv->pLog->Log("[HMD][Hypereal] Could not create compositor: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
// 		return;
// 	}
// 
// 	m_renderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
// 	if (eError != vr::EVRInitError::VRInitError_None)
// 	{
// 		m_renderModels = nullptr;
// 		gEnv->pLog->Log("[HMD][Hypereal] Could not create render models: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
// 		// let's not exit here, because we can live without render models!
// 	}
// 
// 	m_overlay = (vr::IVROverlay*)vr::VR_GetGenericInterface(vr::IVROverlay_Version, &eError);
// 	if (eError != vr::EVRInitError::VRInitError_None)
// 	{
// 		m_overlay = nullptr;
// 		gEnv->pLog->Log("[HMD][Hypereal] Could not create overlay: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
// 	}
// 
// 	float fNear = gEnv->pRenderer->GetCamera().GetNearPlane();
// 	float fFar = gEnv->pRenderer->GetCamera().GetFarPlane();
// 
// 	vr::HmdMatrix44_t proj = m_system->GetProjectionMatrix(vr::EVREye::Eye_Left, fNear, fFar, vr::EGraphicsAPIConvention::API_DirectX);
// 
// 	float fovh = 2.0f * atan(1.0f / proj.m[1][1]);
// 	float aspectRatio = proj.m[1][1] / proj.m[0][0];
// 	float fovv = fovh * aspectRatio;
// 	float asymH = proj.m[0][2] / proj.m[1][1] * aspectRatio;
// 	float asymV = proj.m[1][2] / proj.m[1][1];
// 
// 	vr::IVRSettings* vrSettings = vr::VRSettings();
// 
// 	//int x,y = 0;
// 	//uint w,h = 0;
// 	//m_pSystem->GetWindowBounds(&x,&y,&w,&h);
// 	//m_pSystem->AttachToWindow(gEnv->pRenderer->GetCurrentContextHWND());
// 	//the previous calls to get the compositor window resolution were (temporarily?) dropped from the Hypereal SDK, therefore we report the suggested render resolution as Hmd screen resolution - even though that is NOT correct in the case of the HTC Vive!!!
// 	GetRenderTargetSize(m_devInfo.screenWidth, m_devInfo.screenHeight);
// 	m_devInfo.manufacturer = GetTrackedDeviceCharPointer(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_ManufacturerName_String);
// 	m_devInfo.productName = GetTrackedDeviceCharPointer(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_TrackingSystemName_String);
// 	m_devInfo.fovH = fovh;
// 	m_devInfo.fovV = fovv;
// 
// 	gEnv->pLog->Log("[HMD][Hypereal] - Device: %s", m_devInfo.productName);
// 	gEnv->pLog->Log("[HMD][Hypereal] --- Manufacturer: %s", m_devInfo.manufacturer);
// 	gEnv->pLog->Log("[HMD][Hypereal] --- SerialNumber: %s", GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_SerialNumber_String).c_str());
// 	gEnv->pLog->Log("[HMD][Hypereal] --- Firmware: %s", GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_TrackingFirmwareVersion_String).c_str());
// 	gEnv->pLog->Log("[HMD][Hypereal] --- Hardware Revision: %s", GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_HardwareRevision_String).c_str());
// 	gEnv->pLog->Log("[HMD][Hypereal] --- Display Firmware: %d", m_system->GetUint64TrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_DisplayFirmwareVersion_Uint64));
// 	gEnv->pLog->Log("[HMD][Hypereal] --- Resolution: %dx%d", m_devInfo.screenWidth, m_devInfo.screenHeight);
// 	gEnv->pLog->Log("[HMD][Hypereal] --- Refresh Rate: %.2f", m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_DisplayFrequency_Float));
// 	gEnv->pLog->Log("[HMD][Hypereal] --- FOVH: %.2f", RAD2DEG(fovh));
// 	gEnv->pLog->Log("[HMD][Hypereal] --- FOVV: %.2f", RAD2DEG(fovv));
// 
// #if CRY_PLATFORM_WINDOWS
// 	// the following is (hopefully just) a (temporary) hack to shift focus back to the CryEngine window, after (potentially) spawning the SteamVR Compositor
// 	if (!gEnv->IsEditor())
// 	{
// 		LockSetForegroundWindow(LSFW_UNLOCK);
// 		SetForegroundWindow((HWND)gEnv->pSystem->GetHWND());
// 	}
// #endif
}

void Device::ReleaseDevice() 
{
	HY_RELEASE(VrDevice);

	bVRSystemValid = false;

	if (bVRInitialized)
	{
		bVRInitialized = false;
		HyShutdown();
	}
}
// -------------------------------------------------------------------------
void Device::DebugDraw(float& xPosLabel, float& yPosLabel) const
{
	// Basic info
	const float yPos = yPosLabel, xPosData = xPosLabel, yDelta = 20.f;
	float y = yPos;
	const ColorF fColorLabel(1.0f, 1.0f, 1.0f, 1.0f);
	const ColorF fColorIdInfo(1.0f, 1.0f, 0.0f, 1.0f);
	const ColorF fColorInfo(0.0f, 1.0f, 0.0f, 1.0f);
	const ColorF fColorDataPose(0.0f, 1.0f, 1.0f, 1.0f);

	IRenderAuxText::Draw2dLabel(xPosLabel, y, 1.3f, fColorLabel, false, "Hypereal HMD Info:");
	y += yDelta;

	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorIdInfo, false, "Device:%ss", m_devInfo.productName ? m_devInfo.productName : "unknown");
	y += yDelta;
	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorIdInfo, false, "Manufacturer:%s", m_devInfo.manufacturer ? m_devInfo.manufacturer : "unknown");
	y += yDelta;
	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorInfo, false, "Resolution: %dx%d", m_devInfo.screenWidth, m_devInfo.screenHeight);
	y += yDelta;
	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorInfo, false, "FOV (degrees): H:%.2f - V:%.2f", m_devInfo.fovH * 180.0f / gf_PI, m_devInfo.fovV * 180.0f / gf_PI);
	y += yDelta;

 	const Vec3 hmdPos = m_localStates[EDevice::Hmd].pose.position;
 	const Ang3 hmdRotAng(m_localStates[EDevice::Hmd].pose.orientation);
 	const Vec3 hmdRot(RAD2DEG(hmdRotAng));
 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataPose, false, "HmdPos(LS):[%.f,%f,%f]", hmdPos.x, hmdPos.y, hmdPos.z);
 	y += yDelta;
 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataPose, false, "HmdRot(LS):[%.f,%f,%f] (PRY) degrees", hmdRot.x, hmdRot.y, hmdRot.z);
 	y += yDelta;
 
 	yPosLabel = y;
}

// -------------------------------------------------------------------------
void Device::OnRecentered()
{
	RecenterPose();
}

// -------------------------------------------------------------------------
void Device::RecenterPose()
{
//	m_system->ResetSeatedZeroPose();
}

// -------------------------------------------------------------------------
const HmdTrackingState& Device::GetLocalTrackingState() const
{
	return m_hmdTrackingDisabled ? m_disabledTrackingState : m_localStates[EDevice::Hmd];
}

// -------------------------------------------------------------------------
Quad Device::GetPlayArea() const
{
	Quad result;
	result.vCorners[0] = HmdVec3ToWorldVec3(Vec3(PlayAreaVertices[0].x, PlayAreaVertices[0].y, 0));
	result.vCorners[1] = HmdVec3ToWorldVec3(Vec3(PlayAreaVertices[1].x, PlayAreaVertices[1].y, 0));
	result.vCorners[2] = HmdVec3ToWorldVec3(Vec3(PlayAreaVertices[2].x, PlayAreaVertices[2].y, 0));
	result.vCorners[3] = HmdVec3ToWorldVec3(Vec3(PlayAreaVertices[3].x, PlayAreaVertices[3].y, 0));
	return result;

}

// -------------------------------------------------------------------------
Vec2 Device::GetPlayAreaSize() const
{
// 	if (auto* pChaperone = vr::VRChaperone())
// 	{
// 		Vec2 result;
// 		if (pChaperone->GetPlayAreaSize(&result.x, &result.y))
// 		{
// 			return result;
// 		}
// 	}

	return Vec2(ZERO);
}

// -------------------------------------------------------------------------
const HmdTrackingState& Device::GetNativeTrackingState() const
{
	return m_hmdTrackingDisabled ? m_disabledTrackingState : m_nativeStates[EDevice::Hmd];
}

// -------------------------------------------------------------------------
const EHmdSocialScreen Device::GetSocialScreenType(bool* pKeepAspect) const
{

	const int kFirstInvalidIndex = static_cast<int>(EHmdSocialScreen::FirstInvalidIndex);

	if (pKeepAspect)
	{
		*pKeepAspect = m_pHmdSocialScreenKeepAspectCVar->GetIVal() != 0;
	}

	if (m_pHmdSocialScreenCVar->GetIVal() >= -1 && m_pHmdSocialScreenCVar->GetIVal() < kFirstInvalidIndex)
	{
		const EHmdSocialScreen socialScreenType = static_cast<EHmdSocialScreen>(m_pHmdSocialScreenCVar->GetIVal());
		return socialScreenType;
	}

	return EHmdSocialScreen::UndistortedDualImage; // default to dual distorted image
}

// -------------------------------------------------------------------------
void Device::PrintHmdInfo()
{
	// nada
}

// -------------------------------------------------------------------------
void Device::SubmitOverlay(int id)
{
//  	if (m_mapOverlayers.size()==0)
//  		return;
 
 	if (HyViewLayer* pLayer = m_overlays[id].layerHandle)
 	{
 		m_overlays[id].submitted = true;
 	}
}

// -------------------------------------------------------------------------
void Device::SubmitFrame()
{
	if (VrGraphicsCxt)
	{
		//HyTextureDesc RTDesc[2];



		HyResult hr = hySuccess;
		
// 		RTDesc[0].m_texture = resL->RenderTex;
// 		RTDesc[1].m_texture = resR->RenderTex;
		hr = VrGraphicsCxt->Submit(m_lastFrameID_UpdateTrackingState, RTDesc, 2);


// 		if (!m_mapOverlayers.size()==0)
// 			return;
// 
// 		for (int id = 0; id < RenderLayer::eQuadLayers_Total; id++)
// 		{
// 			if (!m_overlays[id].submitted && m_overlays[id].visible&&m_overlays[id].layerHandle)
// 			{
// 				HyTextureDesc  desc = m_overlays[id].textureDesc;
// 				desc.m_texture = nullptr;
// 				m_overlays[id].layerHandle->SetTexture(desc);
// 
// 				m_overlays[id].visible = false;
// 			}
// 			else if (m_overlays[id].submitted && !m_overlays[id].visible&&m_overlays[id].layerHandle)
// 			{
// 				HyTextureDesc  desc = m_overlays[id].textureDesc;
// 				m_overlays[id].layerHandle->SetTexture(desc);
// 
// 				m_overlays[id].visible = true;
// 			}
// 			m_overlays[id].submitted = false;
// 		}
	}


	
// 	FRAME_PROFILER("Device::SubmitFrame", gEnv->pSystem, PROFILE_SYSTEM);
// 
// 	if (m_compositor && m_eyeTargets[EEyeType::eEyeType_LeftEye] && m_eyeTargets[EEyeType::eEyeType_RightEye])
// 	{
// 		m_compositor->Submit(vr::Eye_Left, m_eyeTargets[EEyeType::eEyeType_LeftEye]);
// 		m_compositor->Submit(vr::Eye_Right, m_eyeTargets[EEyeType::eEyeType_RightEye]);
// 
// 		m_compositor->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
// 	}
// 
// 	if (!m_overlay)
// 		return;
// 
// 	for (int id = 0; id < RenderLayer::eQuadLayers_Total; id++)
// 	{
// 		if (!m_overlays[id].submitted && m_overlays[id].visible)
// 		{
// 			m_overlay->HideOverlay(m_overlays[id].handle);
// 			m_overlays[id].visible = false;
// 		}
// 		else if (m_overlays[id].submitted && !m_overlays[id].visible)
// 		{
// 			m_overlay->ShowOverlay(m_overlays[id].handle);
// 			m_overlays[id].visible = true;
// 		}
// 		m_overlays[id].submitted = false;
// 	}
}

// -------------------------------------------------------------------------
void Device::GetRenderTargetSize(uint& w, uint& h)
{
	if (nullptr == VrGraphicsCxt)
	{
		w = 1200;
		h = 1080;
		return;
	}
	VrGraphicsCxt->GetRenderTargetSize(HY_EYE_LEFT, w, h);
	
// 
// 	w = (uint)VrDeviceInfo.DeviceResolutionX;
// 	h = (uint)VrDeviceInfo.DeviceResolutionY;
}

// -------------------------------------------------------------------------
void Device::GetMirrorImageView(EEyeType eye, void* resource, void** mirrorTextureView)
{
	/*VrGraphicsCxt->CopyMirrorTexture(resource,(uint) VrDeviceInfo.DeviceResolutionX/2, (uint)VrDeviceInfo.DeviceResolutionY);*/
	//vr::VRCompositor()->GetMirrorTextureD3D11(static_cast<vr::EVREye>(eye), resource, mirrorTextureView);
}

// -------------------------------------------------------------------------
void Device::OnSetupEyeTargets(ERenderAPI api, ERenderColorSpace colorSpace, void* leftEyeHandle, void* rightEyeHandle)
{
	RTDesc[EEyeType::eEyeType_LeftEye].m_texture = leftEyeHandle;
	RTDesc[EEyeType::eEyeType_RightEye].m_texture = rightEyeHandle;
// 	m_eyeTargets[EEyeType::eEyeType_LeftEye] = new vr::Texture_t();
// 	m_eyeTargets[EEyeType::eEyeType_RightEye] = new vr::Texture_t();
// 
// 	switch (colorSpace)
// 	{
// 	case CryVR::Hypereal::ERenderColorSpace::eRenderColorSpace_Auto:
// 		m_eyeTargets[EEyeType::eEyeType_LeftEye]->eColorSpace = m_eyeTargets[EEyeType::eEyeType_RightEye]->eColorSpace = vr::EColorSpace::ColorSpace_Auto;
// 		break;
// 	case CryVR::Hypereal::ERenderColorSpace::eRenderColorSpace_Gamma:
// 		m_eyeTargets[EEyeType::eEyeType_LeftEye]->eColorSpace = m_eyeTargets[EEyeType::eEyeType_RightEye]->eColorSpace = vr::EColorSpace::ColorSpace_Gamma;
// 		break;
// 	case CryVR::Hypereal::ERenderColorSpace::eRenderColorSpace_Linear:
// 		m_eyeTargets[EEyeType::eEyeType_LeftEye]->eColorSpace = m_eyeTargets[EEyeType::eEyeType_RightEye]->eColorSpace = vr::EColorSpace::ColorSpace_Linear;
// 		break;
// 	default:
// 		m_eyeTargets[EEyeType::eEyeType_LeftEye]->eColorSpace = m_eyeTargets[EEyeType::eEyeType_RightEye]->eColorSpace = vr::EColorSpace::ColorSpace_Auto;
// 		break;
// 	}
// 
// 	switch (api)
// 	{
// 	case CryVR::Hypereal::ERenderAPI::eRenderAPI_DirectX:
// 		m_eyeTargets[EEyeType::eEyeType_LeftEye]->eType = m_eyeTargets[EEyeType::eEyeType_RightEye]->eType = vr::EGraphicsAPIConvention::API_DirectX;
// 		break;
// 	case CryVR::Hypereal::ERenderAPI::eRenderAPI_OpenGL:
// 		m_eyeTargets[EEyeType::eEyeType_LeftEye]->eType = m_eyeTargets[EEyeType::eEyeType_RightEye]->eType = vr::EGraphicsAPIConvention::API_OpenGL;
// 		break;
// 	default:
// 		m_eyeTargets[EEyeType::eEyeType_LeftEye]->eType = m_eyeTargets[EEyeType::eEyeType_RightEye]->eType = vr::EGraphicsAPIConvention::API_DirectX;
// 		break;
// 	}
// 
// 	m_eyeTargets[EEyeType::eEyeType_LeftEye]->handle = leftEyeHandle;
// 	m_eyeTargets[EEyeType::eEyeType_RightEye]->handle = rightEyeHandle;
}

// -------------------------------------------------------------------------
void Device::OnSetupOverlay(int id, ERenderAPI api, ERenderColorSpace colorSpace, void* overlayTextureHandle)
{
	if (nullptr==VrDevice|| nullptr == VrGraphicsCxt)
		return;

	//remove old one with same id
	MapOverlayer::iterator itFind = m_mapOverlayers.find(id);
	if (itFind != m_mapOverlayers.end())
	{
		HyViewLayer* pLayer = itFind->second.layerHandle;
		pLayer->Release();
		m_mapOverlayers.erase(itFind);
	}

	HyViewLayer* pLayer = (HyViewLayer*)VrDevice->CreateClassInstance(HY_CLASS_VIEW_LAYER);

	if (nullptr == pLayer)
	{
		gEnv->pLog->Log("[HMD][Hypereal] Error creating overlay %i", id);
		return;
	}
	
	SOverlay newOverlayer;
	newOverlayer.layerHandle = pLayer;
	
	Matrix34 matPose =  Matrix34::CreateTranslationMat(Vec3(0, 0, -m_hmdQuadDistance));
	Quat qRot(matPose);

	HyPose pose;
	pose.m_position = Vec3ToHYVec3(matPose.GetTranslation());
	pose.m_rotation = QuatToHYQuat(qRot);
	pLayer->SetPose(pose);
	HyVec2 hySize;
	hySize.x = 1.0f;
	hySize.y = 1.0f;
	pLayer->SetSize(hySize);
	

	HyTextureDesc hyTextureDesc;
	hyTextureDesc.m_texture = overlayTextureHandle;
	hyTextureDesc.m_uvOffset = HyVec2{ 0.0f , 0.0f };
	hyTextureDesc.m_uvSize = HyVec2{ 1.0f , 1.0f };
	hyTextureDesc.m_flags = 0;
	pLayer->SetTexture(hyTextureDesc);
	
	if (m_hmdQuadAbsolute)
		pLayer->SetFlags(0);
	else
		pLayer->SetFlags(HY_LAYER_FLAG_LOCK_TO_HELMET);

	pLayer->SetPriority(id);

	m_overlays[id].visible = false;
	m_overlays[id].submitted = false;
	m_overlays[id].layerHandle = pLayer;
	m_overlays[id].overlayTexture = overlayTextureHandle;
	m_overlays[id].textureDesc = hyTextureDesc;

	m_mapOverlayers[id] = m_overlays[id];

// 	if (!m_overlay)
// 		return;
// 
// 	char keyName[64];
// 	cry_sprintf(keyName, 64, "cry.overlay_%d", id);
// 
// 	char overlayName[64];
// 	cry_sprintf(overlayName, 64, "cryOverlay_%d", id);
// 
// 	vr::VROverlayHandle_t handle;
// 	if (m_overlay->FindOverlay(keyName, &handle) == vr::EVROverlayError::VROverlayError_None)
// 		m_overlay->DestroyOverlay(handle);
// 
// 	vr::EVROverlayError err = m_overlay->CreateOverlay(keyName, overlayName, &m_overlays[id].handle);
// 	if (err != vr::EVROverlayError::VROverlayError_None)
// 	{
// 		gEnv->pLog->Log("[HMD][Hypereal] Error creating overlay %i: %s", id, m_overlay->GetOverlayErrorNameFromEnum(err));
// 		return;
// 	}
// 
// 	m_overlays[id].vrTexture = new vr::Texture_t();
// 	m_overlays[id].vrTexture->handle = overlayTextureHandle;
// 	switch (api)
// 	{
// 	case CryVR::Hypereal::ERenderAPI::eRenderAPI_DirectX:
// 		m_overlays[id].vrTexture->eType = vr::EGraphicsAPIConvention::API_DirectX;
// 		break;
// 	case CryVR::Hypereal::ERenderAPI::eRenderAPI_OpenGL:
// 		m_overlays[id].vrTexture->eType = vr::EGraphicsAPIConvention::API_OpenGL;
// 		break;
// 	default:
// 		m_overlays[id].vrTexture->eType = vr::EGraphicsAPIConvention::API_DirectX;
// 		break;
// 	}
// 	switch (colorSpace)
// 	{
// 	case CryVR::Hypereal::ERenderColorSpace::eRenderColorSpace_Auto:
// 		m_overlays[id].vrTexture->eColorSpace = vr::EColorSpace::ColorSpace_Auto;
// 		break;
// 	case CryVR::Hypereal::ERenderColorSpace::eRenderColorSpace_Gamma:
// 		m_overlays[id].vrTexture->eColorSpace = vr::EColorSpace::ColorSpace_Gamma;
// 		break;
// 	case CryVR::Hypereal::ERenderColorSpace::eRenderColorSpace_Linear:
// 		m_overlays[id].vrTexture->eColorSpace = vr::EColorSpace::ColorSpace_Linear;
// 		break;
// 	default:
// 		m_overlays[id].vrTexture->eColorSpace = vr::EColorSpace::ColorSpace_Auto;
// 		break;
// 	}
// 
// 	m_overlays[id].pos = vr::RawConvert(Matrix34::CreateTranslationMat(Vec3(0, 0, -m_hmdQuadDistance)));
// 
// 	vr::VRTextureBounds_t overlayTextureBounds = { 0, 0, 1.f, 1.f };
// 	m_overlay->SetOverlayTextureBounds(m_overlays[id].handle, &overlayTextureBounds);
// 	m_overlay->SetOverlayTexture(m_overlays[id].handle, m_overlays[id].vrTexture);
// 	if (m_hmdQuadAbsolute)
// 		m_overlay->SetOverlayTransformAbsolute(m_overlays[id].handle, m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor ? vr::ETrackingUniverseOrigin::TrackingUniverseStanding : vr::ETrackingUniverseOrigin::TrackingUniverseSeated, &(m_overlays[id].pos));
// 	else
// 		m_overlay->SetOverlayTransformTrackedDeviceRelative(m_overlays[id].handle, vr::k_unTrackedDeviceIndex_Hmd, &(m_overlays[id].pos));
// 	m_overlay->SetOverlayWidthInMeters(m_overlays[id].handle, m_hmdQuadWidth);
// 	m_overlay->SetHighQualityOverlay(m_overlays[id].handle);
// 	m_overlay->HideOverlay(m_overlays[id].handle);
// 	m_overlays[id].visible = false;
// 	m_overlays[id].submitted = false;
}

// -------------------------------------------------------------------------
void Device::OnDeleteOverlay(int id)
{
	if (!VrDevice&&VrGraphicsCxt)
		return;

	//remove old one with same id
	MapOverlayer::iterator itFind = m_mapOverlayers.find(id);
	if (itFind != m_mapOverlayers.end())
	{
		HyViewLayer* pLayer = itFind->second.layerHandle;
		pLayer->Release();
		m_mapOverlayers.erase(itFind);

		memset(&m_overlays[id], 0, sizeof(m_overlays[id]));
	}

// 	if (!m_overlay)
// 		return;
// 
// 	char keyName[64];
// 	cry_sprintf(keyName, 64, "cry.overlay_%d", id);
// 
// 	char overlayName[64];
// 	cry_sprintf(overlayName, 64, "cryOverlay_%d", id);
// 
// 	vr::VROverlayHandle_t handle;
// 	if (m_overlay->FindOverlay(keyName, &handle) == vr::EVROverlayError::VROverlayError_None)
// 	{
// 		m_overlay->ClearOverlayTexture(handle);
// 		m_overlay->HideOverlay(handle);
// 		m_overlay->DestroyOverlay(handle);
// 
// 	}
// 	SAFE_DELETE(m_overlays[id].vrTexture);
// 	m_overlays[id].visible = false;
// 	m_overlays[id].submitted = false;
}


void Device::RebuildPlayArea()
{
	if (PlayAreaVertices)
	{
		delete[] PlayAreaVertices;
		PlayAreaVertices = nullptr;
	}

	if (hySucceeded(VrDevice->GetIntValue(HY_PROPERTY_CHAPERONE_VERTEX_COUNT_INT, PlayAreaVertexCount)) && PlayAreaVertexCount != 0)
	{
		PlayAreaVertices = new HyVec2[PlayAreaVertexCount];
		HyResult r = VrDevice->GetFloatArray(HY_PROPERTY_CHAPERONE_VERTEX_VEC2_ARRAY, reinterpret_cast<float*>(PlayAreaVertices),(uint) PlayAreaVertexCount * 2);

		bPlayAreaValid = hySucceeded(r);

		if (!bPlayAreaValid)
		{
			delete[] PlayAreaVertices;
			PlayAreaVertices = nullptr;
		}
	}
}

float Device::GetDistance(const HyVec2& P, const HyVec2& PA, const HyVec2& PB)
{
	float xx = PB.x - PA.x;
	float yy = PB.y - PA.y;
	return (-(yy * P.x - xx * P.y + PB.x * PA.y - PB.y * PA.x) / sqrt(xx * xx + yy * yy));
}

void Device::CreateGraphicsContext(void* graphicsDevice)
{
	if (nullptr == VrDevice)
		return;

	//graphic ctx should be ready
	HyGraphicsAPI graphicsAPI = HY_GRAPHICS_UNKNOWN;

	graphicsAPI = HY_GRAPHICS_D3D11;

	{
		VrGraphicsCxtDesc.m_mirrorWidth = m_devInfo.screenWidth /*gEnv->pRenderer->GetWidth()*/;
		VrGraphicsCxtDesc.m_mirrorHeight = m_devInfo.screenHeight;// gEnv->pRenderer->GetHeight();
	}

	VrGraphicsCxtDesc.m_graphicsDevice = graphicsDevice;
	VrGraphicsCxtDesc.m_graphicsAPI = graphicsAPI;
	VrGraphicsCxtDesc.m_pixelFormat = HY_TEXTURE_R8G8B8A8_UNORM_SRGB;
	VrGraphicsCxtDesc.m_pixelDensity = PixelDensity;
	VrGraphicsCxtDesc.m_flags = 0;

	HyResult hr = VrDevice->CreateGraphicsContext(VrGraphicsCxtDesc, &VrGraphicsCxt);
	if (!hySucceeded(hr))
	{
		gEnv->pLog->Log("[HMD][Hypereal] CreateGraphicsContext failed.");
		return;
	}
}

void Device::ReleaseGraphicsContext()
{
	HY_RELEASE(VrGraphicsCxt);
}

} // namespace Hypereal
} // namespace CryVR
