import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import * as Juce from "./juce/index.js";

const controlParameterIndexAnnotation = "data-juce-control-parameter-index";

const sliderDefaults = {
  input: { name: "Input", label: "dB", start: -18, end: 18, normalised: 0.5, scaled: 0 },
  drive: { name: "Drive", label: "x", start: 1, end: 28, normalised: 0.26, scaled: 5 },
  tone: { name: "Tone", label: "%", start: 0, end: 100, normalised: 0.62, scaled: 62 },
  bias: { name: "Bias", label: "%", start: -50, end: 50, normalised: 0.5, scaled: 0 },
  mix: { name: "Mix", label: "%", start: 0, end: 100, normalised: 1, scaled: 100 },
  output: { name: "Output", label: "dB", start: -24, end: 12, normalised: 0.58, scaled: -3 }
};

const defaultModes = ["Warm", "Hard", "Fold", "Crush"];

function clamp(value, min = 0, max = 1) {
  return Math.min(max, Math.max(min, value));
}

function readNormalised(relay, fallback) {
  const value = relay.getNormalisedValue();
  return Number.isFinite(value) ? clamp(value) : fallback;
}

function mergeSliderProperties(properties, fallback) {
  return {
    ...properties,
    name: properties.name || fallback.name,
    label: properties.label || fallback.label,
    start: Number.isFinite(properties.start) ? properties.start : fallback.start,
    end: Number.isFinite(properties.end) ? properties.end : fallback.end,
    numSteps: properties.numSteps > 1 ? properties.numSteps : 1000
  };
}

function formatValue(id, value) {
  if (id === "drive") return `${value.toFixed(value >= 10 ? 1 : 2)}x`;
  if (id === "input" || id === "output") return `${value.toFixed(1)} dB`;
  if (id === "bias") return `${value >= 0 ? "+" : ""}${value.toFixed(0)}%`;
  return `${value.toFixed(0)}%`;
}

function useSliderRelay(id) {
  const fallback = sliderDefaults[id];
  const relay = useMemo(() => Juce.getSliderState(id), [id]);

  const readSnapshot = useCallback(() => {
    const properties = mergeSliderProperties(relay.properties, fallback);
    const scaledValue = relay.getScaledValue();
    const backendHasRelay = window.__JUCE__.initialisationData.__juce__sliders.includes(id);

    return {
      id,
      properties,
      normalised: readNormalised(relay, fallback.normalised),
      scaled: Number.isFinite(scaledValue) && (backendHasRelay || scaledValue !== 0) ? scaledValue : fallback.scaled
    };
  }, [fallback, id, relay]);

  const [snapshot, setSnapshot] = useState(readSnapshot);

  useEffect(() => {
    const update = () => setSnapshot(readSnapshot());
    const valueListener = relay.valueChangedEvent.addListener(update);
    const propertiesListener = relay.propertiesChangedEvent.addListener(update);

    update();

    return () => {
      relay.valueChangedEvent.removeListener(valueListener);
      relay.propertiesChangedEvent.removeListener(propertiesListener);
    };
  }, [readSnapshot, relay]);

  const setNormalised = useCallback((value) => {
    relay.setNormalisedValue(clamp(value));
    setSnapshot(readSnapshot());
  }, [readSnapshot, relay]);

  return {
    ...snapshot,
    setNormalised,
    beginGesture: () => relay.sliderDragStarted(),
    endGesture: () => relay.sliderDragEnded()
  };
}

function useToggleRelay(id) {
  const relay = useMemo(() => Juce.getToggleState(id), [id]);

  const readSnapshot = useCallback(() => ({
    value: relay.getValue(),
    properties: {
      ...relay.properties,
      name: relay.properties.name || "HQ"
    }
  }), [relay]);

  const [snapshot, setSnapshot] = useState(readSnapshot);

  useEffect(() => {
    const update = () => setSnapshot(readSnapshot());
    const valueListener = relay.valueChangedEvent.addListener(update);
    const propertiesListener = relay.propertiesChangedEvent.addListener(update);

    update();

    return () => {
      relay.valueChangedEvent.removeListener(valueListener);
      relay.propertiesChangedEvent.removeListener(propertiesListener);
    };
  }, [readSnapshot, relay]);

  const setValue = useCallback((value) => {
    relay.setValue(Boolean(value));
    setSnapshot(readSnapshot());
  }, [readSnapshot, relay]);

  return { ...snapshot, setValue };
}

function useComboRelay(id, fallbackChoices) {
  const relay = useMemo(() => Juce.getComboBoxState(id), [id]);

  const readSnapshot = useCallback(() => {
    const properties = {
      ...relay.properties,
      name: relay.properties.name || "Mode",
      choices: relay.properties.choices?.length ? relay.properties.choices : fallbackChoices
    };

    return {
      choiceIndex: clamp(relay.getChoiceIndex(), 0, properties.choices.length - 1),
      properties
    };
  }, [fallbackChoices, relay]);

  const [snapshot, setSnapshot] = useState(readSnapshot);

  useEffect(() => {
    const update = () => setSnapshot(readSnapshot());
    const valueListener = relay.valueChangedEvent.addListener(update);
    const propertiesListener = relay.propertiesChangedEvent.addListener(update);

    update();

    return () => {
      relay.valueChangedEvent.removeListener(valueListener);
      relay.propertiesChangedEvent.removeListener(propertiesListener);
    };
  }, [readSnapshot, relay]);

  const setChoiceIndex = useCallback((index) => {
    relay.setChoiceIndex(index);
    setSnapshot(readSnapshot());
  }, [readSnapshot, relay]);

  return { ...snapshot, setChoiceIndex };
}

function useLevels() {
  const [levels, setLevels] = useState({ input: 0, output: 0 });

  useEffect(() => {
    const token = window.__JUCE__.backend.addEventListener("levels", (payload) => {
      setLevels({
        input: clamp(Number(payload.input) || 0),
        output: clamp(Number(payload.output) || 0)
      });
    });

    const hasNativeSliders = window.__JUCE__.initialisationData.__juce__sliders.length > 0;
    let devTimer = null;

    if (!hasNativeSliders) {
      devTimer = window.setInterval(() => {
        const now = performance.now() * 0.001;
        setLevels({
          input: clamp(0.2 + Math.sin(now * 1.7) * 0.12 + Math.sin(now * 6.2) * 0.03),
          output: clamp(0.34 + Math.sin(now * 2.1) * 0.17 + Math.sin(now * 8.3) * 0.04)
        });
      }, 80);
    }

    return () => {
      window.__JUCE__.backend.removeEventListener(token);
      if (devTimer !== null) window.clearInterval(devTimer);
    };
  }, []);

  return levels;
}

function useControlParameterIndexBridge() {
  useEffect(() => {
    const updater = new Juce.ControlParameterIndexUpdater(controlParameterIndexAnnotation);
    const handleMove = (event) => updater.handleMouseMove(event);

    document.addEventListener("mousemove", handleMove);
    return () => document.removeEventListener("mousemove", handleMove);
  }, []);
}

function ShearMark() {
  return (
    <div className="brand-lockup" aria-label="Shear">
      <svg className="shear-mark" viewBox="0 0 96 60" role="img" aria-label="Shear logo">
        <defs>
          <linearGradient id="markTop" x1="12" y1="14" x2="74" y2="44" gradientUnits="userSpaceOnUse">
            <stop offset="0" stopColor="#4bc3b1" />
            <stop offset="1" stopColor="#f7ca61" />
          </linearGradient>
          <linearGradient id="markBlade" x1="52" y1="5" x2="35" y2="55" gradientUnits="userSpaceOnUse">
            <stop offset="0" stopColor="#ff6f53" />
            <stop offset="1" stopColor="#fff5dd" />
          </linearGradient>
        </defs>
        <path className="mark-shadow" d="M13 43h22l7-18h27l14 18h-8L64 30H47l-6 18H13Z" />
        <path className="mark-signal mark-signal-a" d="M10 41h20l8-22h18l6 10h13" />
        <path className="mark-signal mark-signal-b" d="M14 49h24l7-18h18l8 12h15" />
        <path className="mark-blade" d="M58 4h9L39 56h-9Z" />
        <path className="mark-blade-edge" d="M63 5 35 55" />
        <path className="mark-spark" d="M70 18h13M73 26h8" />
      </svg>
      <div>
        <h1>Shear</h1>
        <span>distortion shaper</span>
      </div>
    </div>
  );
}

function Meter({ label, value }) {
  const db = value <= 0.0001 ? -72 : 20 * Math.log10(value);
  const percent = clamp((db + 48) / 48) * 100;

  return (
    <div className="meter">
      <span>{label}</span>
      <div className="meter-track" aria-hidden="true">
        <div className="meter-fill" style={{ height: `${percent}%` }} />
        <div className="meter-hotline" />
      </div>
      <strong>{db <= -71 ? "-inf" : db.toFixed(1)} dB</strong>
    </div>
  );
}

function shapeSample(sample, mode, drive, bias) {
  let x = clamp(sample * drive + bias * 0.01, -8, 8);
  const centredBias = Math.tanh(bias * 0.023) * 0.38;
  let y = 0;

  if (mode === "Hard") y = clamp(x * 0.74, -1, 1);
  else if (mode === "Fold") {
    x = ((x + 3) % 4 + 4) % 4;
    y = (2 - Math.abs(x - 2)) * 2 - 1;
  } else if (mode === "Crush") {
    const limited = Math.tanh(x * 0.8);
    y = Math.round(limited * 24) / 24;
  } else {
    y = Math.tanh(x * 1.15);
  }

  return clamp(y - centredBias, -1, 1);
}

function TransferCurve({ drive, bias, tone, mix, mode, levels }) {
  const canvasRef = useRef(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const bounds = {
      width: canvas.clientWidth,
      height: canvas.clientHeight
    };
    const ratio = window.devicePixelRatio || 1;
    const backingWidth = Math.max(1, Math.round(bounds.width * ratio));
    const backingHeight = Math.max(1, Math.round(bounds.height * ratio));

    if (canvas.width !== backingWidth) canvas.width = backingWidth;
    if (canvas.height !== backingHeight) canvas.height = backingHeight;

    const context = canvas.getContext("2d");
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.clearRect(0, 0, bounds.width, bounds.height);

    const grid = context.createLinearGradient(0, 0, bounds.width, bounds.height);
    grid.addColorStop(0, "rgba(247, 202, 97, 0.18)");
    grid.addColorStop(0.5, "rgba(75, 195, 177, 0.12)");
    grid.addColorStop(1, "rgba(255, 111, 83, 0.16)");
    context.fillStyle = grid;
    context.fillRect(0, 0, bounds.width, bounds.height);

    context.strokeStyle = "rgba(231, 226, 207, 0.12)";
    context.lineWidth = 1;
    for (let i = 1; i < 5; i += 1) {
      const x = (bounds.width / 5) * i;
      const y = (bounds.height / 5) * i;
      context.beginPath();
      context.moveTo(x, 0);
      context.lineTo(x, bounds.height);
      context.moveTo(0, y);
      context.lineTo(bounds.width, y);
      context.stroke();
    }

    const glow = context.createLinearGradient(0, 0, bounds.width, 0);
    glow.addColorStop(0, "#4bc3b1");
    glow.addColorStop(0.45, "#f7ca61");
    glow.addColorStop(1, "#ff6f53");

    context.shadowBlur = 18;
    context.shadowColor = "rgba(247, 202, 97, 0.48)";
    context.strokeStyle = glow;
    context.lineWidth = 3;
    context.beginPath();

    for (let i = 0; i <= 180; i += 1) {
      const input = (i / 180) * 2 - 1;
      const wet = shapeSample(input, mode, drive, bias);
      const dryWet = input * (1 - mix / 100) + wet * (mix / 100);
      const tamed = dryWet * (0.76 + tone / 420);
      const x = (i / 180) * bounds.width;
      const y = bounds.height * (0.5 - tamed * 0.42);

      if (i === 0) context.moveTo(x, y);
      else context.lineTo(x, y);
    }

    context.stroke();
    context.shadowBlur = 0;

    context.fillStyle = "rgba(255, 111, 83, 0.9)";
    context.fillRect(bounds.width - 26, bounds.height * (1 - levels.output), 10, bounds.height * levels.output);
    context.fillStyle = "rgba(75, 195, 177, 0.9)";
    context.fillRect(bounds.width - 42, bounds.height * (1 - levels.input), 10, bounds.height * levels.input);
  }, [bias, drive, levels.input, levels.output, mix, mode, tone]);

  return <canvas ref={canvasRef} className="curve-canvas" aria-label="Transfer curve" />;
}

function KnobControl({ slider, emphasis = false }) {
  const angle = `${slider.normalised * 270 - 135}deg`;
  const dragState = useRef(null);
  const valueText = formatValue(slider.id, slider.scaled);

  const finishGesture = (event) => {
    if (dragState.current === null) return;

    if (event.currentTarget.hasPointerCapture?.(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }

    dragState.current = null;
    slider.endGesture();
  };

  const updateFromPointer = (event) => {
    if (dragState.current === null) return;

    event.preventDefault();

    const { startX, startY, startValue } = dragState.current;
    const dragDelta = (startY - event.clientY) + ((event.clientX - startX) * 0.35);
    const sensitivity = event.shiftKey ? 520 : 220;

    slider.setNormalised(startValue + (dragDelta / sensitivity));
  };

  const handleKeyDown = (event) => {
    const coarseStep = 0.02;
    const fineStep = 0.005;
    const pageStep = 0.1;
    const step = event.shiftKey ? fineStep : coarseStep;
    let nextValue = slider.normalised;

    if (event.key === "ArrowUp" || event.key === "ArrowRight") nextValue += step;
    else if (event.key === "ArrowDown" || event.key === "ArrowLeft") nextValue -= step;
    else if (event.key === "PageUp") nextValue += pageStep;
    else if (event.key === "PageDown") nextValue -= pageStep;
    else if (event.key === "Home") nextValue = 0;
    else if (event.key === "End") nextValue = 1;
    else return;

    event.preventDefault();
    slider.beginGesture();
    slider.setNormalised(nextValue);
    slider.endGesture();
  };

  return (
    <section
      className={`knob-control ${emphasis ? "is-primary" : ""}`}
      style={{ "--angle": angle, "--value": slider.normalised }}
      data-juce-control-parameter-index={slider.properties.parameterIndex}
    >
      <div className="knob-topline">
        <span>{slider.properties.name}</span>
        <strong>{valueText}</strong>
      </div>
      <div className="knob-shell">
        <div className="knob">
          <div className="knob-pointer" />
          <div className="knob-core" />
        </div>
        <div
          className="knob-hit-area"
          role="slider"
          tabIndex="0"
          aria-label={slider.properties.name}
          aria-valuemin={slider.properties.start}
          aria-valuemax={slider.properties.end}
          aria-valuenow={Number(slider.scaled.toFixed(2))}
          aria-valuetext={valueText}
          onPointerDown={(event) => {
            event.preventDefault();
            event.currentTarget.setPointerCapture(event.pointerId);
            dragState.current = {
              startX: event.clientX,
              startY: event.clientY,
              startValue: slider.normalised
            };
            slider.beginGesture();
          }}
          onPointerMove={updateFromPointer}
          onPointerUp={finishGesture}
          onPointerCancel={finishGesture}
          onKeyDown={handleKeyDown}
        />
      </div>
      <div className="knob-scale" aria-hidden="true">
        <span>{slider.properties.start}</span>
        <span>{slider.properties.end}</span>
      </div>
    </section>
  );
}

function ModeSelector({ combo }) {
  const current = Math.round(combo.choiceIndex);

  return (
    <div
      className="mode-selector"
      data-juce-control-parameter-index={combo.properties.parameterIndex}
      aria-label={combo.properties.name}
    >
      {combo.properties.choices.map((choice, index) => (
        <button
          type="button"
          key={choice}
          className={index === current ? "is-active" : ""}
          aria-pressed={index === current}
          onClick={() => combo.setChoiceIndex(index)}
        >
          {choice}
        </button>
      ))}
    </div>
  );
}

function HqToggle({ toggle }) {
  return (
    <button
      type="button"
      className={`hq-toggle ${toggle.value ? "is-on" : ""}`}
      aria-pressed={toggle.value}
      data-juce-control-parameter-index={toggle.properties.parameterIndex}
      onClick={() => toggle.setValue(!toggle.value)}
    >
      <span>{toggle.properties.name}</span>
      <strong>{toggle.value ? "On" : "Off"}</strong>
    </button>
  );
}

function App() {
  useControlParameterIndexBridge();

  const input = useSliderRelay("input");
  const drive = useSliderRelay("drive");
  const tone = useSliderRelay("tone");
  const bias = useSliderRelay("bias");
  const mix = useSliderRelay("mix");
  const output = useSliderRelay("output");
  const mode = useComboRelay("mode", defaultModes);
  const hq = useToggleRelay("hq");
  const levels = useLevels();

  const modeName = mode.properties.choices[Math.round(mode.choiceIndex)] || defaultModes[0];

  return (
    <main className="plugin-shell">
      <header className="top-bar">
        <ShearMark />
        <ModeSelector combo={mode} />
        <HqToggle toggle={hq} />
      </header>

      <section className="workbench">
        <div className="meter-bank">
          <Meter label="In" value={levels.input} />
          <Meter label="Out" value={levels.output} />
        </div>

        <div className="curve-panel">
          <div className="curve-heading">
            <span>{modeName}</span>
            <strong>{hq.value ? "oversampled feel" : "direct path"}</strong>
          </div>
          <TransferCurve
            drive={drive.scaled}
            bias={bias.scaled}
            tone={tone.scaled}
            mix={mix.scaled}
            mode={modeName}
            levels={levels}
          />
        </div>

        <KnobControl slider={drive} emphasis />
      </section>

      <section className="knob-strip" aria-label="Shear controls">
        <KnobControl slider={input} />
        <KnobControl slider={tone} />
        <KnobControl slider={bias} />
        <KnobControl slider={mix} />
        <KnobControl slider={output} />
      </section>
    </main>
  );
}

export default App;
