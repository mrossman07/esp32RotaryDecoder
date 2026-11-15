import CoreBluetooth
import Foundation

/// A push-button gesture classified by the firmware: tap (< 1 s) or long press (>= 1 s).
public enum ButtonGesture: UInt8, Equatable, Sendable {
    case shortTap = 0x01
    case longPress = 0x02
}

/// GATT contract with the firmware. Keep in sync with `docs/ble-protocol.md`
/// and `firmware/lib/EncoderBle`.
public enum EncoderGATT {
    public static let deviceName = "RotaryEnc"

    public static let service = CBUUID(string: "F0B85AB3-94CF-4D88-BF3B-ED017371EDF9")
    /// Read, Notify. `int16` little-endian, -100...100.
    public static let value = CBUUID(string: "6BF5D408-E6A7-4EEF-9AC7-9890E9CC56A6")
    /// Read, Notify. `uint8`: 0 = released, 1 = pressed.
    public static let button = CBUUID(string: "3B371367-7749-4319-A442-AE92F52A406D")
    /// Write. `uint8` command.
    public static let control = CBUUID(string: "CC30C315-5374-4956-B9A6-7826AB23BD93")

    /// Read, Notify. `uint8` event: 0x01 short tap, 0x02 long press.
    public static let gesture = CBUUID(string: "7670A629-C062-4B70-A4DE-09594771DBCB")
    /// Read, Notify. `uint8`: 0 or 1, flipped by the firmware on every long press.
    public static let latch = CBUUID(string: "B429C943-136E-45BB-849C-EEB677D6BF00")

    /// Control command 0x01: reset the value to 0.
    public static let resetCommand = Data([0x01])

    /// Decodes the Gesture characteristic. Unknown codes (including 0 = none) give nil.
    public static func parseGesture(_ data: Data) -> ButtonGesture? {
        guard let byte = data.first else { return nil }
        return ButtonGesture(rawValue: byte)
    }

    /// Decodes the Latch characteristic (`uint8`, nonzero = on).
    public static func parseLatch(_ data: Data) -> Bool? {
        guard let byte = data.first else { return nil }
        return byte != 0
    }

    /// Decodes the Value characteristic (`int16`, little-endian).
    public static func parseValue(_ data: Data) -> Int? {
        guard data.count >= 2 else { return nil }
        let raw = UInt16(data[data.startIndex]) | (UInt16(data[data.startIndex + 1]) << 8)
        return Int(Int16(bitPattern: raw))
    }

    /// Decodes the Button characteristic (`uint8`, nonzero = pressed).
    public static func parseButton(_ data: Data) -> Bool? {
        guard let byte = data.first else { return nil }
        return byte != 0
    }
}
