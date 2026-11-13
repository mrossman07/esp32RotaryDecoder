import Foundation

/// A fake encoder for previews, the iOS simulator (no Bluetooth there) and tests.
public final class MockTransport: EncoderTransport {
    public var onEvent: ((EncoderEvent) -> Void)?

    private let autoDemo: Bool
    private var value = 0
    private var direction = 1
    private var ticks = 0
    private var timer: Timer?

    /// - Parameter autoDemo: when true, the value sweeps up and down and the button
    ///   toggles on its own, so the UI can be seen working without hardware.
    public init(autoDemo: Bool = false) {
        self.autoDemo = autoDemo
    }

    public func start() {
        onEvent?(.state(.scanning))
        onEvent?(.state(.connected))
        onEvent?(.value(value))
        onEvent?(.button(false))
        if autoDemo { startTimer() }
    }

    public func stop() {
        timer?.invalidate()
        timer = nil
        onEvent?(.state(.idle))
    }

    public func sendReset() {
        value = 0
        onEvent?(.value(0))
    }

    /// Simulates turning the knob by `delta` detents (clamped like the firmware).
    public func turn(by delta: Int) {
        value = min(100, max(-100, value + delta))
        onEvent?(.value(value))
    }

    /// Simulates pressing or releasing the button.
    public func press(_ pressed: Bool) {
        onEvent?(.button(pressed))
    }

    private func startTimer() {
        timer = Timer.scheduledTimer(withTimeInterval: 0.15, repeats: true) { [weak self] _ in
            self?.tick()
        }
    }

    private func tick() {
        ticks += 1
        if abs(value) >= 40 { direction = value > 0 ? -1 : 1 }
        turn(by: direction)
        if ticks % 12 == 0 { press(true) }
        if ticks % 12 == 6 { press(false) }
    }
}
