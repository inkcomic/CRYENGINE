#include "StdAfx.h"

#include "HyperealController.h"

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IRenderAuxGeom.h>

namespace CryVR {
namespace Hypereal {

#define VRC_AXIS_THRESHOLD 0.5f

#define MAPSYMBOL(EKI, DEV_KEY_ID, KEY_NAME, KEY_TYPE) m_symbols[EKI - HYPEREAL_BASE] = MapSymbol(DEV_KEY_ID, EKI, KEY_NAME, KEY_TYPE, 0);

// -------------------------------------------------------------------------
Controller::Controller():m_Device(nullptr)
{
	memset(&m_previousState, 0, sizeof(m_previousState));
	memset(&m_state, 0, sizeof(m_state));

	for (int iController = 0; iController < eKI_Motion_Hypereal_NUM_SYMBOLS; iController++)
		m_controllerMapping[iController] = HY_SUBDEV_CONTROLLER_RESERVED;//max controller id
}

// -------------------------------------------------------------------------
bool Controller::Init(HyDevice *device)
{
	m_Device = device;
	// Button symbols
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_A, HyButton::HY_BUTTON_A, "hytouch_a", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_B, HyButton::HY_BUTTON_B, "hytouch_b", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_X, HyButton::HY_BUTTON_X, "hytouch_x", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_Y, HyButton::HY_BUTTON_Y, "hytouch_y", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TriggerBtnL, HyButton::HY_BUTTON_THUMB_LEFT, "hytouch_thumbl", SInputSymbol::Trigger);
	MAPSYMBOL(eKI_Motion_Hypereal_TriggerBtnR, HyButton::HY_BUTTON_THUMB_RIGHT, "hytouch_thumbr", SInputSymbol::Trigger);
	MAPSYMBOL(eKI_Motion_Hypereal_SideTriggerBtnL, HyButton::HY_BUTTON_SHOULDER_LEFT, "hytouch_shoulderl", SInputSymbol::Trigger);
	MAPSYMBOL(eKI_Motion_Hypereal_SideTriggerBtnR, HyButton::HY_BUTTON_SHOULDER_RIGHT, "hytouch_shoulderr", SInputSymbol::Trigger);

	return true;
}

#undef MapSymbol

// -------------------------------------------------------------------------
Controller::~Controller()
{
	for (int iSymbol = 0; iSymbol < eKI_Motion_Hypereal_NUM_SYMBOLS; iSymbol++)
		SAFE_DELETE(m_symbols[iSymbol]);
}

// -------------------------------------------------------------------------
void Controller::ClearState()
{
	for (int i = 0; i < eHmdController_Hypereal_MaxHyperealControllers; i++)
		m_state[i] = SControllerState();
}

// -------------------------------------------------------------------------
SInputSymbol* Controller::MapSymbol(uint32 deviceSpecificId, EKeyId keyId, const TKeyName& name, SInputSymbol::EType type, uint32 user)
{
	SInputSymbol* pSymbol = new SInputSymbol(deviceSpecificId, keyId, name, type);
	pSymbol->deviceType = eIDT_MotionController;
	pSymbol->user = user;
	if (type == SInputSymbol::EType::Axis || type == SInputSymbol::EType::RawAxis)
		pSymbol->state = eIS_Unknown;
	else
		pSymbol->state = eIS_Pressed;
	pSymbol->value = 0.0f;
	return pSymbol;
}

// -------------------------------------------------------------------------
void Controller::Update(HySubDevice controllerId, HmdTrackingState nativeState, HmdTrackingState localState, HyInputState& vrState)
{
 	EHmdController index = eHmdController_Hypereal_MaxHyperealControllers;
 	for (int i = 0; i < eHmdController_Hypereal_MaxHyperealControllers; i++)
 	{
 		if (m_controllerMapping[i] == controllerId)
 		{
 			index = static_cast<EHmdController>(i);
 			break;
 		}
 	}
 
 	if ((index < eHmdController_Hypereal_MaxHyperealControllers)/* && (m_state[index].packetNum != vrState.unPacketNum)*/)
 	{
 		m_previousState[index] = m_state[index];
  		
 		m_state[index].buttonsPressed = vrState.m_buttons;
 
 		m_state[index].trigger = vrState.m_trigger;
		m_state[index].sideTrigger = vrState.m_sideTrigger;
 
 		// Send button events (if necessary)
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_A);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_B);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_X);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_Y);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TriggerBtnL);
		PostButtonIfChanged(index, eKI_Motion_Hypereal_TriggerBtnR);
		PostButtonIfChanged(index, eKI_Motion_Hypereal_SideTriggerBtnL);
		PostButtonIfChanged(index, eKI_Motion_Hypereal_SideTriggerBtnR);
 		// send trigger event (if necessary)
 		if (m_state[index].trigger != m_previousState[index].trigger)
 		{
 			SInputEvent event;
 			SInputSymbol* pSymbol = m_symbols[eKI_Motion_OpenVR_Trigger - HYPEREAL_BASE];
 			pSymbol->ChangeEvent(m_state[index].trigger);
 			pSymbol->AssignTo(event);
 			event.deviceIndex = controllerId;
 			event.deviceType = eIDT_MotionController;
 			gEnv->pInput->PostInputEvent(event);
 		}
 
		// send trigger event (if necessary)
		if (m_state[index].sideTrigger != m_previousState[index].sideTrigger)
		{
			SInputEvent event;
			SInputSymbol* pSymbol = m_symbols[eKI_Motion_OpenVR_Trigger - HYPEREAL_BASE];
			pSymbol->ChangeEvent(m_state[index].sideTrigger);
			pSymbol->AssignTo(event);
			event.deviceIndex = controllerId;
			event.deviceType = eIDT_MotionController;
			gEnv->pInput->PostInputEvent(event);
		}
 	}
 	m_state[index].localPose = localState;
 	m_state[index].nativePose = nativeState;
}

// -------------------------------------------------------------------------
void Controller::PostButtonIfChanged(EHmdController controllerId, EKeyId keyID)
{
	SInputSymbol* pSymbol = m_symbols[keyID - HYPEREAL_BASE];
	HyButton vrBtn = static_cast<HyButton>(pSymbol->devSpecId);

	bool wasPressed = m_previousState[controllerId].Pressed(vrBtn);
	bool isPressed = m_state[controllerId].Pressed(vrBtn);

	if (isPressed != wasPressed)
	{
		SInputEvent event;
		pSymbol->PressEvent(isPressed);
		pSymbol->AssignTo(event);
		event.deviceIndex = controllerId;
		event.deviceType = eIDT_MotionController;
		gEnv->pInput->PostInputEvent(event);
	}
}

// -------------------------------------------------------------------------
void Controller::DebugDraw(float& xPosLabel, float& yPosLabel) const
{
// 	// Basic info
// 	const float controllerWidth = 400.0f;
// 	const float yPos = yPosLabel, xPosData = xPosLabel, yDelta = 20.f;
// 	float y = yPos;
// 	const ColorF fColorLabel(1.0f, 1.0f, 1.0f, 1.0f);
// 	const ColorF fColorDataConn(1.0f, 1.0f, 0.0f, 1.0f);
// 	const ColorF fColorDataBt(0.0f, 1.0f, 0.0f, 1.0f);
// 	const ColorF fColorDataTr(0.0f, 0.0f, 1.0f, 1.0f);
// 	const ColorF fColorDataTh(1.0f, 0.0f, 1.0f, 1.0f);
// 	const ColorF fColorDataPose(0.0f, 1.0f, 1.0f, 1.0f);
// 
// 	IRenderAuxText::Draw2dLabel(xPosLabel, y, 1.3f, fColorLabel, false, "Hypereal Controller Info:");
// 	y += yDelta;
// 
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataConn, false, "Left Hand:%s", IsConnected(eHmdController_OpenVR_1) ? "Connected" : "Disconnected");
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataConn, false, "Right Hand:%s", IsConnected(eHmdController_OpenVR_2) ? "Connected" : "Disconnected");
// 	y += yDelta;
// 
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataBt, false, "System:%s", IsButtonPressed(eHmdController_OpenVR_1, eKI_Motion_OpenVR_System) ? "Pressed" : "Released");
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataBt, false, "Bt System:%s", IsButtonPressed(eHmdController_OpenVR_2, eKI_Motion_OpenVR_System) ? "Pressed" : "Released");
// 	y += yDelta;
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataBt, false, "App:%s", IsButtonPressed(eHmdController_OpenVR_1, eKI_Motion_OpenVR_ApplicationMenu) ? "Pressed" : "Released");
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataBt, false, "Bt App:%s", IsButtonPressed(eHmdController_OpenVR_2, eKI_Motion_OpenVR_ApplicationMenu) ? "Pressed" : "Released");
// 	y += yDelta;
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataBt, false, "Grip:%s", IsButtonPressed(eHmdController_OpenVR_1, eKI_Motion_OpenVR_Grip) ? "Pressed" : "Released");
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataBt, false, "Bt Grip:%s", IsButtonPressed(eHmdController_OpenVR_2, eKI_Motion_OpenVR_Grip) ? "Pressed" : "Released");
// 	y += yDelta;
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataBt, false, "Trigger:%s", IsButtonPressed(eHmdController_OpenVR_1, eKI_Motion_OpenVR_TriggerBtn) ? "Pressed" : "Released");
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataBt, false, "Bt Trigger:%s", IsButtonPressed(eHmdController_OpenVR_2, eKI_Motion_OpenVR_TriggerBtn) ? "Pressed" : "Released");
// 	y += yDelta;
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataBt, false, "Pad:%s", IsButtonPressed(eHmdController_OpenVR_1, eKI_Motion_OpenVR_TouchPadBtn) ? "Pressed" : "Released");
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataBt, false, "Bt Pad:%s", IsButtonPressed(eHmdController_OpenVR_2, eKI_Motion_OpenVR_TouchPadBtn) ? "Pressed" : "Released");
// 	y += yDelta;
// 
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataTr, false, "Trigger:%.2f", GetTriggerValue(eHmdController_OpenVR_1, eKI_Motion_OpenVR_Trigger));
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataTr, false, "Trigger:%.2f", GetTriggerValue(eHmdController_OpenVR_2, eKI_Motion_OpenVR_Trigger));
// 	y += yDelta;
// 
// 	Vec2 touchLeft = GetThumbStickValue(eHmdController_OpenVR_1, eKI_Motion_OpenVR_TouchPad_X);
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataTh, false, "Pad:(%.2f %.2f)", touchLeft.x, touchLeft.y);
// 	Vec2 touchRight = GetThumbStickValue(eHmdController_OpenVR_2, eKI_Motion_OpenVR_TouchPad_X);
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataTh, false, "Pad:(%.2f %.2f)", touchRight.x, touchRight.y);
// 	y += yDelta;
// 
// 	Vec3 posLH = GetNativeTrackingState(eHmdController_OpenVR_1).pose.position;
// 	Vec3 posRH = GetNativeTrackingState(eHmdController_OpenVR_2).pose.position;
// 	Vec3 posWLH = GetLocalTrackingState(eHmdController_OpenVR_1).pose.position;
// 	Vec3 posWRH = GetLocalTrackingState(eHmdController_OpenVR_2).pose.position;
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataPose, false, "Pose:(%.2f,%.2f,%.2f)", posLH.x, posLH.y, posLH.z);
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataPose, false, "Pose:(%.2f,%.2f,%.2f)", posRH.x, posRH.y, posRH.z);
// 	y += yDelta;
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataPose, false, "W-Pose:(%.2f,%.2f,%.2f)", posWLH.x, posWLH.y, posWLH.z);
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataPose, false, "W-Pose:(%.2f,%.2f,%.2f)", posWRH.x, posWRH.y, posWRH.z);
// 	y += yDelta;
// 
// 	Ang3 rotLHAng(GetNativeTrackingState(eHmdController_OpenVR_1).pose.orientation);
// 	Vec3 rotLH(RAD2DEG(rotLHAng));
// 	Ang3 rotRHAng(GetNativeTrackingState(eHmdController_OpenVR_2).pose.orientation);
// 	Vec3 rotRH(RAD2DEG(rotRHAng));
// 
// 	Ang3 rotWLHAng(GetLocalTrackingState(eHmdController_OpenVR_1).pose.orientation);
// 	Vec3 rotWLH(RAD2DEG(rotWLHAng));
// 	Ang3 rotWRHAng(GetLocalTrackingState(eHmdController_OpenVR_2).pose.orientation);
// 	Vec3 rotWRH(RAD2DEG(rotWRHAng));
// 
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataPose, false, "Rot[PRY]:(%.2f,%.2f,%.2f)", rotLH.x, rotLH.y, rotLH.z);
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataPose, false, "Rot[PRY]:(%.2f,%.2f,%.2f)", rotRH.x, rotRH.y, rotRH.z);
// 	y += yDelta;
// 	IRenderAuxText::Draw2dLabel(xPosData, y, 1.3f, fColorDataPose, false, "W-Rot[PRY]:(%.2f,%.2f,%.2f)", rotWLH.x, rotWLH.y, rotWLH.z);
// 	IRenderAuxText::Draw2dLabel(xPosData + controllerWidth, y, 1.3f, fColorDataPose, false, "W-Rot[PRY]:(%.2f,%.2f,%.2f)", rotWRH.x, rotWRH.y, rotWRH.z);
// 	y += yDelta;
// 
// 	yPosLabel = y;
}

// -------------------------------------------------------------------------;
void Controller::OnControllerConnect(HySubDevice controllerId)
{
	bool added = false;
	for (int i = 0; i < eHmdController_Hypereal_MaxHyperealControllers; i++)
	{
		if (m_controllerMapping[i] >= HY_SUBDEV_CONTROLLER_RESERVED)
		{
			m_controllerMapping[i] = controllerId;
			m_state[i] = SControllerState();
			m_previousState[i] = SControllerState();
			added = true;
			break;
		}
	}

	if (!added)
	{
		gEnv->pLog->LogError("[HMD][Hypereal] Maximum number of controllers reached! Ignoring new controller!");
	}
}

// -------------------------------------------------------------------------
void Controller::OnControllerDisconnect(HySubDevice controllerId)
{
	for (int i = 0; i < eHmdController_OpenVR_MaxNumOpenVRControllers; i++)
	{
		if (m_controllerMapping[i] == controllerId)
		{
			m_controllerMapping[i] = HY_SUBDEV_CONTROLLER_RESERVED;
			break;
		}
	}
}

// -------------------------------------------------------------------------
bool Controller::IsConnected(EHmdController id) const
{
	bool bControllerConnected = false;
	if (eHmdController_Hypereal_1==id)
		m_Device->GetBoolValue(HY_PROPERTY_CONTROLLER0_CONNECTED_BOOL, bControllerConnected);
	else if(eHmdController_Hypereal_2==id)
		m_Device->GetBoolValue(HY_PROPERTY_CONTROLLER0_CONNECTED_BOOL, bControllerConnected);
	return bControllerConnected;
}

// -------------------------------------------------------------------------
void Controller::ApplyForceFeedback(EHmdController id, float freq, float amplitude)
{
	auto durationMicroSeconds = (ushort)clamp_tpl(freq, 0.f, 5000.f);
	m_Device->SetControllerVibration(m_controllerMapping[id], durationMicroSeconds, amplitude);
}

} // namespace Hypereal
} // namespace CryVR
