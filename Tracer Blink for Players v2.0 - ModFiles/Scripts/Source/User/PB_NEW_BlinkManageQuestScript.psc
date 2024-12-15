ScriptName PB_NEW_BlinkManageQuestScript Extends Quest

Actor Property PlayerRef Auto
EffectShader Property PF_FlashEffect Auto

String Property Flash_Widget = "PB_BlinkInterface.swf" AutoReadOnly
Quest Property NoUSE_PB_BlinkManageQuest Auto

Message Property PB_msg_NonHDFramework Auto
Message Property PB_msg_iniFileHotkey_NonActive Auto

GLobalVariable Property gINIFileHotkeyEnable Auto
GLobalVariable Property gBlinkAPCost Auto
GLobalVariable Property gBlinkDuration Auto
GLobalVariable Property g_iFlashStack Auto

GLobalVariable Property g_fOffsetXRatio Auto
GLobalVariable Property g_fOffsetYRatio Auto

int Property HUD_Print = 10 AutoReadOnly
int Property HUD_IconChange = 20 AutoReadOnly

int Property HUD_Type_OverWatch = 0 AutoReadOnly
int Property HUD_Type_Battery = 1 AutoReadOnly
int Property HUD_Type_Slice = 2 AutoReadOnly
int Property HUD_Type_Nest = 3 AutoReadOnly

int iFlashStack = 3
int iTimerID = 6974
int iSkipDebug

float fScaleBaseX = 0.35
float fScaleBaseZ = 0.35
float fScaleBaseBaseZ = 0.35

float Property fScaleLength = 1.0 Auto
float Property fChargeTime = 2.5 Auto
float Property fRange = 5000.0 Auto
float Property fScale = 1.0 Auto
int Property iHUDType Auto

int Property iPosX = 30 Auto
int Property iPosZ = 30 Auto

bool bPosChange
bool bScaleChange
bool bRangeChange
bool bChargeChange
bool bIconChange

HUDFramework hud

Function fIconChange()
	if !bIconChange
		bIconChange = true
		Utility.Wait(0.03)

		iSkipDebug += 1
		HUD.SendMessage(Flash_Widget, HUD_IconChange, iHUDType, iSkipDebug)
		Utility.Wait(0.1)
		iSkipDebug += 1
		HUD.SendMessage(Flash_Widget, HUD_Print, g_iFlashStack.GetValue(), iSkipDebug)
		bIconChange = false
	Endif
EndFunction

Function fPosChange()
	if !bPosChange
		bPosChange = true
		Utility.Wait(0.03)

		Var[] args = new Var[2]
		args[0] = -200;
		args[1] = -200;
		UI.Invoke("blinkHUDUI", "root.changePosition", args)

		;;hud.SetWidgetPosition(Flash_Widget, iPosX, iPosZ)
		bPosChange = false
	Endif
EndFunction

Function fScaleChange()
	if !bScaleChange
		bScaleChange = true
		Utility.Wait(0.03)
		fScaleBaseZ = fScaleBaseBaseZ * fScaleLength

		hud.SetWidgetScale(Flash_Widget, fScaleBaseX * fScale, fScaleBaseZ * fScale)
		bScaleChange = false
	Endif
EndFunction

Function fChargeChange()
	if !bChargeChange
		bChargeChange = true
		Utility.Wait(0.03)
		bChargeChange = false
	Endif
EndFunction

Event OnQuestInit()
	hud = HUDFramework.GetInstance()
    	RegisterForRemoteEvent(PlayerRef, "OnPlayerLoadGame")

	iFlashStack = 3

	If (hud)
		iHUDType = HUD_Type_OverWatch

		hud.RegisterWidget(Self, Flash_Widget, iPosX, iPosZ, abLoadNow = True, abAutoLoad = True)
		Utility.Wait(0.1)
		hud.SetWidgetScale(Flash_Widget, fScaleBaseX * fScale, fScaleBaseZ * fScale)
	Else
		PB_msg_NonHDFramework.Show()
	EndIf
EndEvent

Event Actor.OnPlayerLoadGame(Actor akSender)
	iFlashStack = 3
	iSkipDebug = 0

	if gINIFileHotkeyEnable.GetValue() == 1
		PB_PlayerBlink_F4SE.SetINIFileHotkey()
	Endif

	hud.UnRegisterWidget(Flash_Widget)
	Stop()
EndEvent

Function HUD_WidgetLoaded(string asWidget)
	if (asWidget == Flash_Widget)
		iSkipDebug += 1
		HUD.SendMessage(Flash_Widget, HUD_IconChange, iHUDType, iSkipDebug)
		Utility.Wait(0.1)
		HUD.SendMessage(Flash_Widget, HUD_Print, g_iFlashStack.GetValue(), 56)
	EndIf
EndFunction

Function FlashStart()
	bool bBlinkRun = PB_PlayerBlink_F4SE.startBlink()
	if bBlinkRun
		HUD.SendMessage(Flash_Widget, HUD_Print, g_iFlashStack.GetValue(), iSkipDebug)
		StartTimer(fChargeTime, iTimerID)
	Endif
EndFunction

Event OnTimer(int aiID)
	if g_iFlashStack.GetValue() < 3
		iSkipDebug += 1
		if g_iFlashStack.mod(1) < 3
			StartTimer(fChargeTime, iTimerID)
		Endif
		HUD.SendMessage(Flash_Widget, HUD_Print, g_iFlashStack.GetValue(), iSkipDebug)
	Endif
EndEvent

Function RegisterIniFileKey()
	bool setHotkey = PB_PlayerBlink_F4SE.SetINIFileHotkey()
	if !setHotkey
		debug.messagebox("This setting is for users who wish to use a key that cannot be configured within the MCM.\nThe 'useiniFileHotkey' value in the mod folder/F4SE/Plugins/blinkHotkey.ini file is set to 0. \nSince the value is 0, the hotkey registration is canceled. Set it to 1 to enable the hotkey.")
		Return
	else
		Debug.Messagebox("The hotkey for Blink mode has been set.")
	endif
EndFunction
