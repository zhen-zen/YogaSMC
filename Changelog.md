YogaSMC Changelog
============================

#### v1.4.1
- ThinkVPC: fix LEDSupport evaluation, thx @tylernguyen
- NC: add AudioHelper workaround
- NC: fix holiday date, thx @1Revenger1

#### v1.4.0
- Pane: debug: enable RapidChargeMode checkbox
- ThinkVPC: fix forgotten break in switch case, thx @junaedahmed
- NC: update presets, thx @antoniomcr96 @Ab2774
- NC: support localization
- ThinkVPC: check LED availability
- NC: support hideText for actions with customized OSD
- Build: switch ARCHS to x86_64
- ThinkVPC: support yoga mode detection
- NC: support DisableFan option
- YogaHIDD: fix support for devices without `_DSM`
- YogaVPC: validate clamshellCap and updateBacklight support
- YogaWMI: fix getNotifyID
- IdeaWMI: fix getBatteryInfo retain count
- NC: hide selected text which status is revealed by image
- YogaVPC: notify timestamp for key press
- NC: fix easter egg and enable it by default
- YogaWMI: evaluate all possible BMF names
- YogaSMC: support atomicSpDeciKelvinKey
- Docs: merge SSDT samples for ThinkSMC; add suggestions for LNUX
- Pane: separate different views and update when reopen
- YogaSMC: support atomicSpDeciKelvinKey
- NC: update resources, thx @hexart
- NC: support multiple instance, specifically YogaHIDD
- NC: support modifier
- NC: support Caps Lock monitor
- ThinkVPC: evaluate GMKS for FnLock state
- NC: add launchapp action
- YogaBaseService: add identifier for Alter version
- NC: fix ECCap detection, thanks @buyddy
- Pane: support ClamshellMode
- Pane: fix battery detection
- NC: improve audio status handling
- NC: support SaveFanLevel
- IdeaVPC: workaround for buggy event evaluation
- NC: improve config readability, backup your customizations first! 

#### v1.3.0
- Pane: fix loading prior to 10.14.4 with embedded swift runtime, thx @Charlyo
- NC: update presets, thx @junaedahmed @Sniki @tylernguyen
- NC: support dual fan speed reading and control for think variant, thx @1Revenger1
- Pane: fix LED Automation, thx @junaedahmed
- Build: fix short version display
- YogaSMC: pause polling during sleep
- UserClient: recover read by name and write support
- Build: use commit hash as CFBundleVersion
- Build: adapt to Lilu kern_version
- NC: add easter egg for menubar icon
- Pane: support customized menubar title
- YogaVPC: handle WMI probing
- NC: optimize menu handling
- NC: override unknown events with updated presets
- BaseService: support extended EC operation
- YogaSMC: add constants for fan support
- NC: Using cached wireless status, thx @H15teve
- YogaHIDD: experimental support for Intel HID event & 5 button array driver 
- BaseService: support sending key to primary keyboard and palm rejection
- IdeaVPC: poll hotkey status with buggy EC notification with `-ideabr`, thx @moutorde
- Pane: phase out muteCheck for release branch
- ThinkVPC: fix legacy hotkey mask update, thx @junaedahmed

Change of earlier versions may be tracked in [commits](https://github.com/zhen-zen/YogaSMC/commits/master) and [releases](https://github.com/zhen-zen/YogaSMC/releases).
