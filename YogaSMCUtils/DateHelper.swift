//
//  DateHelper.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/31/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation
import AppKit

// based on https://stackoverflow.com/questions/4311930/list-of-all-american-holidays-as-nsdates
func setHolidayIcon(_ button: NSStatusBarButton) {
    button.title = "⎇"
    button.toolTip = nil

    if applyHolidayIconLunar(), setHolidayIconLunar(button) { return }

    let components = Calendar.current.dateComponents([.year, .month, .day, .weekday, .weekdayOrdinal], from: Date())
    guard let year = components.year,
        let month = components.month,
        let day = components.day,
        let weekday = components.weekday,
        let weekdayOrdinal = components.weekdayOrdinal else { return }

    let easterDateComponents = dateComponentsForEaster(year: year)
    let easterMonth: Int = easterDateComponents?.month ?? -1
    let easterDay: Int = easterDateComponents?.day ?? -1

    switch (month, day, weekday, weekdayOrdinal) {
    case (1, 1, _, _):
        button.title = "🎍"
        button.toolTip = NSLocalizedString("New Year", comment: "")
    case (2, 14, _, _):
        button.title = "💝"
        button.toolTip = NSLocalizedString("Valentine's Day", comment: "")
    case (3, 17, _, _):
        button.title = "🍀"
        button.toolTip = NSLocalizedString("Saint Patrick's Day", comment: "")
    case (easterMonth, easterDay, _, _):
        button.title = "🥚"
        button.toolTip = NSLocalizedString("Easter", comment: "")
    case (10, 31, _, _):
        button.title = "🎃"
        button.toolTip = NSLocalizedString("Halloween", comment: "")
    case (11, _, 5, 4):
        button.title = "🦃"
        button.toolTip = NSLocalizedString("Thanksgiving", comment: "")
    case (12, 24, _, _):
        button.title = "🎄"
        button.toolTip = NSLocalizedString("Christmas Eve", comment: "")
    case (12, 25, _, _):
        button.title = "🎁"
        button.toolTip = NSLocalizedString("Christmas Day", comment: "")
    case (12, 31, _, _):
        button.title = "🎇"
        button.toolTip = NSLocalizedString("New Year's Eve", comment: "")
    default:
        break
    }
}

func dateComponentsForEaster(year: Int) -> DateComponents? {
    // Easter calculation from Anonymous Gregorian algorithm
    // AKA Meeus/Jones/Butcher algorithm
    let a = year % 19
    let b = Int(floor(Double(year) / 100))
    let c = year % 100
    let d = Int(floor(Double(b) / 4))
    let e = b % 4
    let f = Int(floor(Double(b+8) / 25))
    let g = Int(floor(Double(b-f+1) / 3))
    let h = (19*a + b - d - g + 15) % 30
    let i = Int(floor(Double(c) / 4))
    let k = c % 4
    let L = (32 + 2*e + 2*i - h - k) % 7
    let m = Int(floor(Double(a + 11*h + 22*L) / 451))
    var dateComponents = DateComponents()
    dateComponents.month = Int(floor(Double(h + L - 7*m + 114) / 31))
    dateComponents.day = ((h + L - 7*m + 114) % 31) + 1
    dateComponents.year = year

    guard let easter = Calendar.current.date(from: dateComponents) else { return nil }
    // Convert to calculate weekday, weekdayOrdinal
    return Calendar.current.dateComponents([.year, .month, .day, .weekday, .weekdayOrdinal], from: easter)
}

func applyHolidayIconLunar() -> Bool {
    if let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC"),
       let res = defaults.object(forKey: "Lunar") as? Bool { return res }

    for lang in Locale.preferredLanguages {
        if lang.hasPrefix("zh") { return true }
    }
    return false
}

func setHolidayIconLunar(_ button: NSStatusBarButton) -> Bool {
    let calendar: Calendar = Calendar(identifier: .chinese)
    let components = calendar.dateComponents([.year, .month, .day], from: Date())
    guard let year = components.year,
        let month = components.month,
        let day = components.day else { return false }

    switch (month, day) {
    case (1, 1):
        button.title = iconForLunarYear(year: year)
        button.title = "🧨"
        button.toolTip = NSLocalizedString("Lunar New Year", comment: "")
    case (1, 15):
        button.title = "🏮"
        button.toolTip = NSLocalizedString("Lantern Festival", comment: "")
    case (5, 5):
        button.title = "🐲"
        button.toolTip = NSLocalizedString("Dragon Festival", comment: "")
    case (7, 7):
        button.title = "🎋"
        button.toolTip = NSLocalizedString("Qixi Festival", comment: "")
    case (7, 15):
        button.title = "🪔"
        button.toolTip = NSLocalizedString("Ghost Festival", comment: "")
    case (8, 15):
        button.title = "🥮"
        button.toolTip = NSLocalizedString("Mid-Autumn Festival", comment: "")
    case (9, 9):
        button.title = "🌼"
        button.toolTip = NSLocalizedString("Double Ninth Festival", comment: "")
    case (12, 8):
        button.title = "🥣"
        button.toolTip = NSLocalizedString("Laba Festival", comment: "")
    case (12, 30):
        button.title = "🎇"
        button.toolTip = NSLocalizedString("Lunar New Year's Eve", comment: "")
    default:
        let formatter = DateFormatter()
        formatter.calendar = calendar
        formatter.dateStyle = .medium
        button.toolTip = formatter.string(from: Date())
        return false
    }
    return true
}

func iconForLunarYear(year: Int) -> String {
    switch year % 12 {
    case 1:
        return "🐀"
    case 2:
        return "🐂"
    case 3:
        return "🐅"
    case 4:
        return "🐇"
    case 5:
        return "🐉"
    case 6:
        return "🐍"
    case 7:
        return "🐎"
    case 8:
        return "🐑"
    case 9:
        return "🐒"
    case 10:
        return "🐓"
    case 11:
        return "🐕"
    case 0:
        return "🐖"
    default:
        return "🧨"
    }
}
