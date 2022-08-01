//
//  YogaSMC-Bridging-Header.h
//  YogaSMC
//
//  Created by Zhen on 10/14/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaSMC_Bridging_Header_h
#define YogaSMC_Bridging_Header_h

#import <Foundation/Foundation.h>
#import <OSD/OSDManager.h>
#import <CoreGraphics/CoreGraphics.h>

#import "../YogaSMC/YogaSMCUserClient.h"
#import "../YogaSMC/YogaVPC/ThinkEvents.h"

// from https://github.com/toy/blueutil/blob/master/blueutil.m#L44
int IOBluetoothPreferencesAvailable();

int IOBluetoothPreferenceGetControllerPowerState();
void IOBluetoothPreferenceSetControllerPowerState(int state);

int IOBluetoothPreferenceGetDiscoverableState();
void IOBluetoothPreferenceSetDiscoverableState(int state);

// from https://github.com/koekeishiya/yabai/issues/147
CG_EXTERN void CoreDockSendNotification(CFStringRef, void*);

#endif /* YogaSMC_Bridging_Header_h */
