import EncoderBLE
import SwiftUI

@main
struct EncoderDemoApp: App {
    @State private var model = EncoderModel(transport: EncoderDemoApp.makeTransport())

    var body: some Scene {
        WindowGroup {
            ContentView(model: model)
                .onAppear { model.start() }
        }
    }

    /// The iOS simulator has no Bluetooth, so it gets an animated fake encoder.
    /// Pass `-mock` as a launch argument to use the fake on a real device or Mac too.
    private static func makeTransport() -> EncoderTransport {
        #if targetEnvironment(simulator)
        return MockTransport(autoDemo: true)
        #else
        if ProcessInfo.processInfo.arguments.contains("-mock") {
            return MockTransport(autoDemo: true)
        }
        return CoreBluetoothTransport()
        #endif
    }
}
