//
//  PropertyHelper.swift
//  YogaSMC
//
//  Created by Zhen on 10/14/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation

func getBoolean(_ key: String, _ service: io_service_t) -> Bool {
    guard let rvalue = IORegistryEntryCreateCFProperty(service, key as CFString, kCFAllocatorDefault, 0),
          let val = rvalue.takeRetainedValue() as? Bool else {
        return false
    }
    return val
}

func getNumber(_ key: String, _ service: io_service_t) -> Int {
    guard let rvalue = IORegistryEntryCreateCFProperty(service, key as CFString, kCFAllocatorDefault, 0),
          let val = rvalue.takeRetainedValue() as? Int else {
        return -1
    }
    return val
}

func getString(_ key: String, _ service: io_service_t) -> String? {
    guard let rvalue = IORegistryEntryCreateCFProperty(service, key as CFString, kCFAllocatorDefault, 0),
          let val = rvalue.takeRetainedValue() as? NSString else {
        return nil
    }
    return val as String
}

func getDictionary(_ key: String, _ service: io_service_t) -> NSDictionary? {
    guard let rvalue = IORegistryEntryCreateCFProperty(service, key as CFString, kCFAllocatorDefault, 0) else {
        return nil
    }
    return rvalue.takeRetainedValue() as? NSDictionary
}

func sendBoolean(_ key: String, _ value: Bool, _ service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(service, key as CFString, value as CFBoolean))
}

func sendNumber(_ key: String, _ value: Int, _ service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(service, key as CFString, value as CFNumber))
}

func sendString(_ key: String, _ value: String, _ service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(service, key as CFString, value as CFString))
}
