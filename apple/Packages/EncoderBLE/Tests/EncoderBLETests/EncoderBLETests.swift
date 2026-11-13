import XCTest
@testable import EncoderBLE

final class GATTTests: XCTestCase {
    func testUUIDsMatchProtocolDoc() {
        XCTAssertEqual(EncoderGATT.service.uuidString.lowercased(), "f0b85ab3-94cf-4d88-bf3b-ed017371edf9")
        XCTAssertEqual(EncoderGATT.value.uuidString.lowercased(), "6bf5d408-e6a7-4eef-9ac7-9890e9cc56a6")
        XCTAssertEqual(EncoderGATT.button.uuidString.lowercased(), "3b371367-7749-4319-a442-ae92f52a406d")
        XCTAssertEqual(EncoderGATT.control.uuidString.lowercased(), "cc30c315-5374-4956-b9a6-7826ab23bd93")
    }

    func testParseValueLittleEndianSigned() {
        XCTAssertEqual(EncoderGATT.parseValue(Data([0x00, 0x00])), 0)
        XCTAssertEqual(EncoderGATT.parseValue(Data([0x01, 0x00])), 1)
        XCTAssertEqual(EncoderGATT.parseValue(Data([0x64, 0x00])), 100)
        XCTAssertEqual(EncoderGATT.parseValue(Data([0xFF, 0xFF])), -1)
        XCTAssertEqual(EncoderGATT.parseValue(Data([0x9C, 0xFF])), -100)
        // The hex seen from the real device in nRF Connect: E0FF is -32.
        XCTAssertEqual(EncoderGATT.parseValue(Data([0xE0, 0xFF])), -32)
    }

    func testParseValueRejectsShortData() {
        XCTAssertNil(EncoderGATT.parseValue(Data()))
        XCTAssertNil(EncoderGATT.parseValue(Data([0x01])))
    }

    func testParseValueHandlesSlicedData() {
        let backing = Data([0xAA, 0x05, 0x00, 0xBB])
        XCTAssertEqual(EncoderGATT.parseValue(backing[1...2]), 5)
    }

    func testParseButton() {
        XCTAssertEqual(EncoderGATT.parseButton(Data([0x00])), false)
        XCTAssertEqual(EncoderGATT.parseButton(Data([0x01])), true)
        XCTAssertNil(EncoderGATT.parseButton(Data()))
    }
}

@MainActor
final class ModelTests: XCTestCase {
    func testStartsIdleAtZero() {
        let model = EncoderModel(transport: MockTransport())
        XCTAssertEqual(model.value, 0)
        XCTAssertFalse(model.buttonPressed)
        XCTAssertEqual(model.state, .idle)
    }

    func testConnectsAndTracksValueAndButton() {
        let mock = MockTransport()
        let model = EncoderModel(transport: mock)
        model.start()
        XCTAssertEqual(model.state, .connected)

        mock.turn(by: 3)
        XCTAssertEqual(model.value, 3)
        mock.turn(by: -5)
        XCTAssertEqual(model.value, -2)

        mock.press(true)
        XCTAssertTrue(model.buttonPressed)
        mock.press(false)
        XCTAssertFalse(model.buttonPressed)
    }

    func testResetReturnsToZero() {
        let mock = MockTransport()
        let model = EncoderModel(transport: mock)
        model.start()
        mock.turn(by: 42)
        model.reset()
        XCTAssertEqual(model.value, 0)
    }

    func testMockClampsLikeFirmware() {
        let mock = MockTransport()
        let model = EncoderModel(transport: mock)
        model.start()
        mock.turn(by: 500)
        XCTAssertEqual(model.value, 100)
        mock.turn(by: -500)
        XCTAssertEqual(model.value, -100)
    }

    func testDisconnectClearsButton() {
        let mock = MockTransport()
        let model = EncoderModel(transport: mock)
        model.start()
        mock.press(true)
        model.handle(.state(.scanning))
        XCTAssertFalse(model.buttonPressed)
    }
}
