import Foundation
import Observation

public enum ConnectionState: Equatable, Sendable {
    case idle
    case bluetoothOff
    case unauthorized
    case unsupported
    case scanning
    case connecting
    case connected

    public var text: String {
        switch self {
        case .idle: return "Not started"
        case .bluetoothOff: return "Bluetooth is off"
        case .unauthorized: return "Bluetooth permission denied"
        case .unsupported: return "Bluetooth not supported"
        case .scanning: return "Searching for encoder…"
        case .connecting: return "Connecting…"
        case .connected: return "Connected"
        }
    }
}

public enum EncoderEvent: Equatable, Sendable {
    case state(ConnectionState)
    case value(Int)
    case button(Bool)
    case gesture(ButtonGesture)
    case latch(Bool)
}

/// The BLE layer. `CoreBluetoothTransport` talks to the real device and `MockTransport`
/// simulates one (SwiftUI previews, the iOS simulator which has no Bluetooth, tests).
/// Events must be delivered on the main thread.
public protocol EncoderTransport: AnyObject {
    var onEvent: ((EncoderEvent) -> Void)? { get set }
    func start()
    func stop()
    func sendReset()
}

/// What the UI observes. The firmware owns the value (clamped to -100...100);
/// the app only displays it.
@MainActor
@Observable
public final class EncoderModel {
    public private(set) var value = 0
    public private(set) var buttonPressed = false
    /// Incremented on every short tap; the UI flashes its "tap" LED when this changes.
    public private(set) var shortTapCount = 0
    /// Toggled by the firmware on every long press; the UI shows it as the "long press" LED.
    public private(set) var latched = false
    public private(set) var state: ConnectionState = .idle

    @ObservationIgnored private let transport: EncoderTransport

    public init(transport: EncoderTransport) {
        self.transport = transport
        transport.onEvent = { [weak self] event in
            // Transports deliver on the main thread.
            MainActor.assumeIsolated { self?.handle(event) }
        }
    }

    public func start() { transport.start() }
    public func stop() { transport.stop() }

    /// Asks the device to reset its value to 0. The new value arrives as a notification.
    public func reset() { transport.sendReset() }

    func handle(_ event: EncoderEvent) {
        switch event {
        case .state(let newState):
            state = newState
            if newState != .connected { buttonPressed = false }
        case .value(let v):
            value = v
        case .button(let pressed):
            buttonPressed = pressed
        case .gesture(let gesture):
            // A long press needs no counter: its effect arrives as a latch update.
            if gesture == .shortTap { shortTapCount += 1 }
        case .latch(let on):
            latched = on
        }
    }
}
