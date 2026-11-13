import EncoderBLE
import SwiftUI

struct ContentView: View {
    let model: EncoderModel

    var body: some View {
        VStack(spacing: 28) {
            Text(model.state.text)
                .font(.subheadline)
                .foregroundStyle(model.state == .connected ? .green : .secondary)

            Text("\(model.value)")
                .font(.system(size: 120, weight: .bold, design: .rounded))
                .monospacedDigit()
                .minimumScaleFactor(0.4)
                .lineLimit(1)
                .contentTransition(.numericText(value: Double(model.value)))
                .animation(.snappy(duration: 0.15), value: model.value)
                .opacity(model.state == .connected ? 1 : 0.4)
                .accessibilityLabel("Encoder value \(model.value)")

            Gauge(value: Double(model.value), in: -100...100) {
                Text("Value")
            } currentValueLabel: {
                EmptyView()
            } minimumValueLabel: {
                Text("-100")
            } maximumValueLabel: {
                Text("100")
            }
            .gaugeStyle(.linearCapacity)

            LEDView(lit: model.buttonPressed)

            Button("Reset to 0") { model.reset() }
                .buttonStyle(.bordered)
                .disabled(model.state != .connected)
        }
        .padding(24)
        .frame(maxWidth: 480)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        #if os(macOS)
        .frame(minWidth: 340, minHeight: 520)
        #endif
    }
}

/// A round "LED" that lights while the encoder's push button is held down.
struct LEDView: View {
    let lit: Bool

    var body: some View {
        Text("LED")
            .font(.headline)
            .foregroundStyle(lit ? Color.black.opacity(0.75) : Color.secondary)
            .frame(width: 88, height: 88)
            .background(
                Circle().fill(lit ? Color.green : Color.gray.opacity(0.25))
            )
            .overlay(Circle().strokeBorder(Color.secondary.opacity(0.5), lineWidth: 2))
            .shadow(color: lit ? Color.green.opacity(0.8) : .clear, radius: 18)
            .animation(.easeOut(duration: 0.1), value: lit)
            .accessibilityLabel("LED")
            .accessibilityValue(lit ? "on" : "off")
    }
}

#Preview("Connected") {
    ContentView(model: previewModel())
}

@MainActor
private func previewModel() -> EncoderModel {
    let mock = MockTransport(autoDemo: true)
    let model = EncoderModel(transport: mock)
    model.start()
    return model
}
