import CoreBluetooth
import Foundation

/// Talks to the real encoder: scans for the encoder service, connects, subscribes to
/// Value and Button, and reconnects automatically if the link drops.
/// All CoreBluetooth callbacks run on the main queue.
public final class CoreBluetoothTransport: NSObject, EncoderTransport {
    public var onEvent: ((EncoderEvent) -> Void)?

    private var central: CBCentralManager?
    private var peripheral: CBPeripheral?
    private var controlCharacteristic: CBCharacteristic?
    private var wantRunning = false

    public override init() {
        super.init()
    }

    public func start() {
        wantRunning = true
        if let central {
            handle(managerState: central.state)
        } else {
            // Creating the manager triggers the Bluetooth permission prompt.
            central = CBCentralManager(delegate: self, queue: .main)
        }
    }

    public func stop() {
        wantRunning = false
        central?.stopScan()
        if let peripheral { central?.cancelPeripheralConnection(peripheral) }
        peripheral = nil
        controlCharacteristic = nil
        emit(.idle)
    }

    public func sendReset() {
        guard let peripheral, let controlCharacteristic else { return }
        peripheral.writeValue(EncoderGATT.resetCommand, for: controlCharacteristic, type: .withResponse)
    }

    // MARK: - Helpers

    private func emit(_ state: ConnectionState) {
        onEvent?(.state(state))
    }

    private func handle(managerState: CBManagerState) {
        switch managerState {
        case .poweredOn:
            if wantRunning { startScan() }
        case .poweredOff:
            emit(.bluetoothOff)
        case .unauthorized:
            emit(.unauthorized)
        case .unsupported:
            emit(.unsupported)
        default:
            break  // .unknown / .resetting: a state update follows
        }
    }

    private func startScan() {
        guard let central, central.state == .poweredOn else { return }
        peripheral = nil
        controlCharacteristic = nil
        emit(.scanning)
        central.scanForPeripherals(withServices: [EncoderGATT.service])
    }
}

// MARK: - CBCentralManagerDelegate

extension CoreBluetoothTransport: CBCentralManagerDelegate {
    public func centralManagerDidUpdateState(_ central: CBCentralManager) {
        handle(managerState: central.state)
    }

    public func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                               advertisementData: [String: Any], rssi RSSI: NSNumber) {
        guard wantRunning else { return }
        central.stopScan()
        self.peripheral = peripheral  // keep a strong reference or the connection is dropped
        peripheral.delegate = self
        emit(.connecting)
        central.connect(peripheral)
    }

    public func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.discoverServices([EncoderGATT.service])
    }

    public func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral,
                               error: Error?) {
        if wantRunning { startScan() }
    }

    public func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral,
                               error: Error?) {
        if wantRunning { startScan() }
    }
}

// MARK: - CBPeripheralDelegate

extension CoreBluetoothTransport: CBPeripheralDelegate {
    public func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == EncoderGATT.service }) else { return }
        peripheral.discoverCharacteristics(
            [EncoderGATT.value, EncoderGATT.button, EncoderGATT.control], for: service)
    }

    public func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService,
                           error: Error?) {
        for characteristic in service.characteristics ?? [] {
            switch characteristic.uuid {
            case EncoderGATT.value, EncoderGATT.button:
                peripheral.setNotifyValue(true, for: characteristic)
                peripheral.readValue(for: characteristic)  // current state on connect
            case EncoderGATT.control:
                controlCharacteristic = characteristic
            default:
                break
            }
        }
        emit(.connected)
    }

    public func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic,
                           error: Error?) {
        guard error == nil, let data = characteristic.value else { return }
        switch characteristic.uuid {
        case EncoderGATT.value:
            if let v = EncoderGATT.parseValue(data) { onEvent?(.value(v)) }
        case EncoderGATT.button:
            if let b = EncoderGATT.parseButton(data) { onEvent?(.button(b)) }
        default:
            break
        }
    }
}
