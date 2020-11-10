YogaSMC Changelog
============================

#### v1.2.1
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
