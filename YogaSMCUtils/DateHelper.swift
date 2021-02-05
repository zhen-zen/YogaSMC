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
func setHolidayIcon(_ button: NSStatusItem) {
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
        button.title = "⎇"
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
