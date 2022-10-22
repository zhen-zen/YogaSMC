YogaSMC Changelog
============================
#### v1.5.3
- YogaBMF: fix kp caused by variable objects parsing

#### v1.5.2
- YogaVPC: query available DYTC functions
- YogaVPC: fix updateAll call hierarchy, thx @antoniomcr96
- ThinkVPC: support SCPF method (DEBUG)
- Pane: adjust text color for Idea variant capability
- ThinkVPC: don't disable top case when Yoga mode changes with clamshell mode enabled, thx @antoniomcr96
- YogaBaseService: publish VersionInfo in alter version, thx @marianopela
- ThinkVPC: remove deprecated methods
- YogaBaseService: rework findPNP; minor fixes
- DYVPC: correct comments for command type
- IdeaWMI: support an extension of Yoga mode control device
- YogaWMI: simplify WMI parsing
- YogaVPC: fix battery notification
- IdeaVPC: simplify capabilities update
- Project: lower executable target to 10.10 (drop console log)
- YogaVPC: fix notification delivery
- ThinkVPC: filter invalid value
- YogaVPC: use evaluateIntegerParam to call methods with only 1 parameter
- ThinkVPC: implement setFnLock as an override function
- IdeaVPC: ignore return of SBMC and SALS
- IdeaVPC: fix conservation backlight update in release build
- YogaVPC: support DYTC PSC function, thx @Someone52 @1Revenger1
- ThinkVPC: skip Yoga mode notification when Clamshell mode is enabled, thx @antoniomcr96
- YogaWMI: temporary fix for missing qualifiers in methods
- IdeaWMI: support super resolution device and assign to FnLock
- YogaWMI: fix clang analysis during mof decompress
- YogaBMF: return OSString from parse_string; fix remaining memory issues
- YogaBMF: unify log format; add more data typpr
- YogaBMF: simplify method handling; remove debug properties
- YogaSMCUtils: fix broken header; rename deprecated APIs
- DYVPC: validate input data size; expose WMIQuery (DEBUG)
- IdeaVPC: check adapter status to isolate keyboard backlight event
- YogaVPC: support setting timeout on keyboard backlight

#### v1.5.1
- YogaSMC: refactor updateEC
- YogaWMI: fix panic in release build, thx @Chris2fourlaw
- YogaWMI: postpone late start to 2s
- YogaSMC: fix kernel panic when setPowerState is called before poller initialized, thx @Chris2fourlaw
- IdeaVPC: add workaround to extract battery info, thx @Chris2fourlaw @SukkaW

#### v1.5.0
- IdeaVPC: wait for EC region init
- IdeaVPC: set conservationModeLock via Battery flag; update keyboard and battery capability via reset
- ThinkVPC: only turn off mic mute led during init
- YogaWMI: skip TB interface by _UID
- ThinkVPC: evaluate raw tablet mode, thx @antoniomcr96
- ThinkVPC: support basic fan speed control through setProperties, thx @wrobeljakub
- YogaBaseService: rework findPNP
- YogaWMI: various improvements
- YogaSMC: store preset with struct
- DYVPC: initial event support
- DYWMI: initial event support
- DYSMC: support sensor reading, thx @jqqqqqqqqqq
- YogaWMI: remove intermediate class
- YogaWMI: fix duplicated loading
- Project: adjust deployment target
- YogaBaseService: fix logging internal name
- YogaSMC: validate EC availability
- YogaSMC: support read internal status
- DYSMC: fix conflict with DirectECKey, thx @jqqqqqqqqqq
- NC: Add Docking Station Notification Events, thx @Sniki
- NC: add icons for dock / undock
- DYSMC: add more sensor names
- YogaWMI: replace checkEvent wth registerEvent
- YogaWMI: locate BMF blob with GUID
- YogaWMI: fix probe timeout on WMI-based VPC
- DYSMC: validate wmis presence
- DYWMI: BIOS event support
- BaseService: simplify PM support check
- IdeaWMI: Game Zone sensor support
- YogaSMC: simplify message handling
- BaseService: adjust log prefix
- YogaVPC: skip tb interface with GUID
- YogaVPC: optimize WMI handling
- YogaWMI: switch to evaluateMethod and queryBlock
- IdeaVPC: show raw battery info (DEBUG)

#### v1.4.3
- YogaWMI: fix BMF parsing

#### v1.4.2
- YogaWMI: complete event handling
- NC: use timer for distinguish input method switch, thx @vnln
- NC: simplify RFHelper handling
- IdeaVPC: support toggle Always On USB
- Project: cleanup
- YogaWMI: seperate BMF validation
- NC: update holiday list
- ThinkVPC: debug:  support keyboard locale
- IdeaVPC: notify battery on conservation mode change
- NC: support launchbundle, thx @simprecicchiani
- Pane: display threshold for all three battery types, thx @antoniomcr96

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
