//
//  ContentView.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/11/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import SwiftUI

struct ContentView: View {
    let version: String
    var body: some View {
        VStack {
            Text("Kext: \(version)")
                .padding()
            Button("Quit", action: { NSApp.terminate(self)
            }).padding()
        }.frame(maxWidth: /*@START_MENU_TOKEN@*/.infinity/*@END_MENU_TOKEN@*/, minHeight: /*@START_MENU_TOKEN@*/0/*@END_MENU_TOKEN@*/,maxHeight: /*@START_MENU_TOKEN@*/.infinity/*@END_MENU_TOKEN@*/)
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView(version: "Unknown")
    }
}
