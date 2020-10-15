//
//  PropertyHelper.swift
//  YogaSMC
//
//  Created by Zhen on 10/14/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation

func getBoolean(_ key: String, _ io_service: io_service_t) -> Bool {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return false
    }

    return rvalue.takeRetainedValue() as! Bool
}

func getNumber(_ key: String, _ io_service: io_service_t) -> Int {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return -1
    }

    return rvalue.takeRetainedValue() as! Int
}

func getString(_ key: String, _ io_service: io_service_t) -> String? {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return nil
    }

    return rvalue.takeRetainedValue() as! NSString as String
}

func getDictionary(_ key: String, _ io_service: io_service_t) -> NSDictionary? {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return nil
    }

    return rvalue.takeRetainedValue() as? NSDictionary
}

func sendBoolean(_ key: String, _ value: Bool, _ io_service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFBoolean))
}

func sendNumber(_ key: String, _ value: Int, _ io_service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFNumber))
}

func sendString(_ key: String, _ value: String, _ io_service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFString))
}

