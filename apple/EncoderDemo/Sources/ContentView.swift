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

            HStack(spacing: 48) {
                // Left: flashes briefly on each short tap of the encoder button.
                VStack(spacing: 8) {
                    FlashingLED(count: model.shortTapCount, color: .green)
                        .accessibilityLabel("Tap LED")
                    Text("Tap").font(.caption).foregroundStyle(.secondary)
                }
                // Right: toggles on each long press (hold for 1 s or more).
                VStack(spacing: 8) {
                    LEDView(lit: model.latched, color: .orange)
                        .accessibilityLabel("Long press LED")
                        .accessibilityValue(model.latched ? "on" : "off")
                    Text("Long press").font(.caption).foregroundStyle(.secondary)
                }
            }

            Button("Reset to 0") { model.reset() }
                .buttonStyle(.bordered)
                .disabled(model.state != .connected)
        }
        .padding(24)
        .frame(maxWidth: 480)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        #if os(macOS)
        .frame(minWidth: 340, minHeight: 540)
        #endif
    }
}

/// A round "LED" that is lit while `lit` is true.
struct LEDView: View {
    let lit: Bool
    var color: Color = .green

    var body: some View {
        Text("LED")
            .font(.headline)
            .foregroundStyle(lit ? Color.black.opacity(0.75) : Color.secondary)
            .frame(width: 88, height: 88)
            .background(
                Circle().fill(lit ? color : Color.gray.opacity(0.25))
            )
            .overlay(Circle().strokeBorder(Color.secondary.opacity(0.5), lineWidth: 2))
            .shadow(color: lit ? color.opacity(0.8) : .clear, radius: 18)
            .animation(.easeOut(duration: 0.1), value: lit)
    }
}

/// An LED that lights for a short moment each time `count` changes.
struct FlashingLED: View {
    let count: Int
    var color: Color = .green
    var duration: Duration = .milliseconds(250)

    @State private var lit = false
    @State private var generation = 0

    var body: some View {
        LEDView(lit: lit, color: color)
            .onChange(of: count) { _, _ in
                lit = true
                generation += 1
                let mine = generation
                Task {
                    try? await Task.sleep(for: duration)
                    // A newer flash owns the LED now; only the latest one turns it off.
                    if generation == mine { lit = false }
                }
            }
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
