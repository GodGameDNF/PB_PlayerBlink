ScriptName PB_BlinkManageQuestScript Extends Quest

Actor Property PlayerRef Auto
EffectShader Property PF_FlashEffect Auto

Quest Property NoUSE_PB_BlinkManageQuest Auto

Message Property PB_msg_NonHDFramework AUto
Message Property PB_msg_iniFileHotkey_NonActive Auto

GLobalVariable Property gINIFileHotkeyEnable Auto

String Property Flash_Widget = "PB_BlinkInterface.swf" AutoReadOnly

int Property Command_UpdateCount01 = 100 AutoReadOnly
int Property Command_UpdateCount02 = 200 AutoReadOnly

HUDFramework hud

int iFlashStack = 3
int iTimerID = 6974
int iSkipDebug

float fScaleBaseX = 1.0
float fScaleBaseZ = 1.0

float Property fScaleLength = 1.0 Auto
float Property fChargeTime = 2.5 Auto
float Property fRange = 5000.0 Auto
float Property fScale = 1.0 Auto

int Property iPosX = 30 Auto
int Property iPosZ = 70 Auto

bool Property bUION = false Auto

bool bPosChange
bool bScaleChange
bool bRangeChange
bool bChargeChange
bool bUIONChange

int iCountType

bool bModStart

Function fPosChange()
	if !bPosChange
		bPosChange = true
		Utility.Wait(0.03)
		hud.SetWidgetPosition(Flash_Widget, iPosX, iPosZ)
		bPosChange = false
	Endif
EndFunction

Function fScaleChange()
	if !bScaleChange
		bScaleChange = true
		Utility.Wait(0.03)
		fScaleBaseZ = 1.0 * fScaleLength

		hud.SetWidgetScale(Flash_Widget, fScaleBaseX * fScale, fScaleBaseZ * fScale)
		bScaleChange = false
	Endif
EndFunction

Function fRangeChange()
	if !bRangeChange
		bRangeChange = true
		Utility.Wait(0.03)
		bRangeChange = false
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

EndEvent

Event Actor.OnPlayerLoadGame(Actor akSender)
	iSkipDebug = 0
	hud.UnRegisterWidget(Flash_Widget)
	Stop()
EndEvent

Function HUD_WidgetLoaded(string asWidget)
	if (asWidget == Flash_Widget)
		HUD.SendMessage(Flash_Widget, iCountType, 11, 55)
		HUD.SendMessage(Flash_Widget, Command_UpdateCount01, 10, 56)
	EndIf
EndFunction

Function FlashStart()
	if iFlashStack > 0
		iFlashStack -= 1
		iSkipDebug += 1
		HUD.SendMessage(Flash_Widget, iCountType, iFlashStack, iSkipDebug)

		float x01 = PlayerRef.x
		float y01 = PlayerRef.y
		PF_FlashEffect.Play(PlayerRef, 0.5)


		float x02 = PlayerRef.x - x01
		float y02 = PlayerRef.y - y01

		float fLength = Math.Pow((x02 * x02) + (y02 * y02), 0.5)

		float xGo = x02/fLength
		float yGo = y02/fLength

		ActorVelocityFramework.SetVelocity(PlayerRef, xGo * fRange, yGO * fRange, 0, 0.1, xGO * fRange, yGO * fRange, 0)

		StartTimer(fChargeTime, iTimerID)
	Endif
EndFunction

Event OnTimer(int aiID)
	if iFlashStack < 3
		iFlashStack += 1
		iSkipDebug += 1
		if iFlashStack < 3
			StartTimer(fChargeTime, iTimerID)
		Endif
		HUD.SendMessage(Flash_Widget, iCountType, iFlashStack, iSkipDebug)
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
