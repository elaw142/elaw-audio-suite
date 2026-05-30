import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import * as Juce from "./juce/index.js";

const controlParameterIndexAnnotation = "data-juce-control-parameter-index";

function clamp(value, min = 0, max = 1) {
  return Math.min(max, Math.max(min, value));
}

function nativeSliderIds() {
  return window.__JUCE__?.initialisationData?.__juce__sliders ?? [];
}

function readNormalised(relay, fallback) {
  const value = relay.getNormalisedValue();
  return Number.isFinite(value) ? clamp(value) : fallback.normalised;
}

function mergeSliderProperties(properties, fallback, backendHasRelay) {
  return {
    ...properties,
    name: properties.name || fallback.name,
    label: properties.label || fallback.label,
    start: backendHasRelay && Number.isFinite(properties.start) ? properties.start : fallback.start,
    end: backendHasRelay && Number.isFinite(properties.end) ? properties.end : fallback.end,
    numSteps: backendHasRelay && properties.numSteps > 1 ? properties.numSteps : 1000
  };
}

function formatValue(control, value) {
  const format = control.format || "percent";
  if (format === "db") return `${value.toFixed(1)} dB`;
  if (format === "x") return `${value.toFixed(value >= 10 ? 1 : 2)}x`;
  if (format === "signedPercent") return `${value >= 0 ? "+" : ""}${value.toFixed(0)}%`;
  if (format === "ratio") return `${value.toFixed(value >= 10 ? 1 : 2)}:1`;
  if (format === "ms") return `${value >= 100 ? value.toFixed(0) : value.toFixed(1)} ms`;
  if (format === "hz") return value >= 1000 ? `${(value / 1000).toFixed(2)} kHz` : `${value.toFixed(0)} Hz`;
  if (format === "hz1") return `${value.toFixed(value >= 1 ? 2 : 2)} Hz`;
  if (format === "semitone") return `${value >= 0 ? "+" : ""}${value.toFixed(1)} st`;
  return `${value.toFixed(0)}%`;
}

function formatScaleValue(control, value) {
  const format = control.format || "percent";
  if (format === "hz") return value >= 1000 ? `${(value / 1000).toFixed(value >= 10000 ? 0 : 1)}k` : `${value.toFixed(0)}`;
  if (format === "hz1") return value >= 1 ? `${value.toFixed(1)}` : `${value.toFixed(2)}`;
  if (format === "ms") return value >= 100 ? `${value.toFixed(0)}` : `${value.toFixed(1)}`;
  if (format === "ratio") return `${value.toFixed(value >= 10 ? 0 : 1)}`;
  if (format === "x") return `${value.toFixed(value >= 10 ? 0 : 1)}`;
  return `${value.toFixed(0)}`;
}

function useSliderRelay(control) {
  const relay = useMemo(() => Juce.getSliderState(control.id), [control.id]);

  const readSnapshot = useCallback(() => {
    const scaledValue = relay.getScaledValue();
    const backendHasRelay = nativeSliderIds().includes(control.id);
    const properties = mergeSliderProperties(relay.properties, control, backendHasRelay);

    return {
      id: control.id,
      control,
      properties,
      normalised: readNormalised(relay, control),
      scaled: Number.isFinite(scaledValue) && (backendHasRelay || scaledValue !== 0) ? scaledValue : control.scaled
    };
  }, [control, relay]);

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

function useToggleRelay(toggle) {
  const relay = useMemo(() => Juce.getToggleState(toggle.id), [toggle.id]);

  const readSnapshot = useCallback(() => ({
    id: toggle.id,
    value: relay.getValue(),
    properties: {
      ...relay.properties,
      name: relay.properties.name || toggle.name
    }
  }), [relay, toggle]);

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

    const hasNativeSliders = nativeSliderIds().length > 0;
    let devTimer = null;

    if (!hasNativeSliders) {
      devTimer = window.setInterval(() => {
        const now = performance.now() * 0.001;
        setLevels({
          input: clamp(0.22 + Math.sin(now * 1.5) * 0.12 + Math.sin(now * 7.2) * 0.035),
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

function ProductMark({ product }) {
  return (
    <div className="brand-lockup" aria-label={product.name}>
      <svg className="suite-mark" viewBox="0 0 96 60" role="img" aria-label={`${product.name} logo`}>
        <defs>
          <linearGradient id={`${product.slug}-mark`} x1="10" y1="6" x2="86" y2="54" gradientUnits="userSpaceOnUse">
            <stop offset="0" stopColor={product.accents[0]} />
            <stop offset="0.55" stopColor={product.accents[1]} />
            <stop offset="1" stopColor={product.accents[2]} />
          </linearGradient>
        </defs>
        <path className="mark-ghost" d="M12 43h18l10-25h17l9 12h18" />
        <path className="mark-main" stroke={`url(#${product.slug}-mark)`} d={markPath(product.slug)} />
        <path className="mark-cut" d="M61 5 35 55" />
        <path className="mark-spark" d={sparkPath(product.slug)} />
      </svg>
      <div>
        <p>Elaw Audio</p>
        <h1>{product.name}</h1>
        <span>{product.tagline}</span>
      </div>
    </div>
  );
}

function markPath(slug) {
  if (slug === "tilt") return "M10 43h18l10-20h14l12 20h22";
  if (slug === "clamp") return "M12 18h18l7 24h23l8-24h16";
  if (slug === "drift") return "M10 34c13-25 22-25 35 0s22 25 41 0";
  if (slug === "rift") return "M11 45 29 17l15 20 13-24 25 34";
  return "M10 41h20l8-22h18l6 10h23";
}

function sparkPath(slug) {
  if (slug === "tilt") return "M70 20h12M74 28h8";
  if (slug === "clamp") return "M26 12h10M60 48h12";
  if (slug === "drift") return "M72 18c6 4 8 8 9 13";
  if (slug === "rift") return "M69 16h14M74 25h8";
  return "M70 18h13M73 26h8";
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

function Visualizer({ product, sliders, modeName, levels }) {
  const canvasRef = useRef(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const bounds = { width: canvas.clientWidth, height: canvas.clientHeight };
    const ratio = window.devicePixelRatio || 1;
    const backingWidth = Math.max(1, Math.round(bounds.width * ratio));
    const backingHeight = Math.max(1, Math.round(bounds.height * ratio));
    if (canvas.width !== backingWidth) canvas.width = backingWidth;
    if (canvas.height !== backingHeight) canvas.height = backingHeight;

    const context = canvas.getContext("2d");
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.clearRect(0, 0, bounds.width, bounds.height);

    const bg = context.createLinearGradient(0, 0, bounds.width, bounds.height);
    bg.addColorStop(0, "rgba(255,255,255,0.055)");
    bg.addColorStop(0.55, "rgba(255,255,255,0.018)");
    bg.addColorStop(1, "rgba(255,255,255,0.04)");
    context.fillStyle = bg;
    context.fillRect(0, 0, bounds.width, bounds.height);

    context.strokeStyle = "rgba(241,234,216,0.115)";
    context.lineWidth = 1;
    for (let i = 1; i < 6; i += 1) {
      const x = (bounds.width / 6) * i;
      const y = (bounds.height / 5) * i;
      context.beginPath();
      context.moveTo(x, 0);
      context.lineTo(x, bounds.height);
      if (i < 5) {
        context.moveTo(0, y);
        context.lineTo(bounds.width, y);
      }
      context.stroke();
    }

    const line = context.createLinearGradient(0, 0, bounds.width, 0);
    line.addColorStop(0, product.accents[0]);
    line.addColorStop(0.55, product.accents[1]);
    line.addColorStop(1, product.accents[2]);
    context.shadowBlur = 20;
    context.shadowColor = product.accents[1];
    context.strokeStyle = line;
    context.lineWidth = 3;
    context.beginPath();

    const get = (id, fallback = 0) => sliders[id]?.scaled ?? fallback;
    for (let i = 0; i <= 220; i += 1) {
      const t = i / 220;
      const x = t * bounds.width;
      const centered = t * 2 - 1;
      let yValue = centered;

      if (product.visual === "transfer") {
        const drive = get("drive", 5);
        const bias = get("bias", 0) * 0.01;
        if (modeName === "Hard") yValue = clamp((centered * drive + bias) * 0.74, -1, 1);
        else if (modeName === "Fold") yValue = 1 - Math.abs(((centered * drive + 3) % 4) - 2);
        else if (modeName === "Crush") yValue = Math.round(Math.tanh(centered * drive) * 18) / 18;
        else yValue = Math.tanh(centered * drive * 1.15 + bias);
      } else if (product.visual === "tilt") {
        const tilt = get("tilt", 0) / 100;
        yValue = Math.sin(t * Math.PI * 2) * 0.22 + centered * tilt * 0.72;
      } else if (product.visual === "clamp") {
        const threshold = Math.abs(get("threshold", -22)) / 48;
        yValue = Math.sign(centered) * Math.min(Math.abs(centered), 1 - threshold * 0.48);
      } else if (product.visual === "drift") {
        const depth = get("depth", 42) / 100;
        const rate = get("rate", 0.75);
        yValue = Math.sin(t * Math.PI * 4 + performance.now() * 0.001 * rate) * (0.3 + depth * 0.45);
      } else {
        const density = get("density", 48) / 100;
        yValue = Math.sin((Math.floor(t * (8 + density * 26)) / (8 + density * 26)) * Math.PI * 8) * 0.72;
      }

      const y = bounds.height * (0.5 - yValue * 0.38);
      if (i === 0) context.moveTo(x, y);
      else context.lineTo(x, y);
    }

    context.stroke();
    context.shadowBlur = 0;

    context.fillStyle = product.accents[2];
    context.fillRect(bounds.width - 24, bounds.height * (1 - levels.output), 8, bounds.height * levels.output);
    context.fillStyle = product.accents[0];
    context.fillRect(bounds.width - 38, bounds.height * (1 - levels.input), 8, bounds.height * levels.input);
  }, [levels.input, levels.output, modeName, product, sliders]);

  return <canvas ref={canvasRef} className="curve-canvas" aria-label={`${product.name} signal display`} />;
}

function KnobControl({ slider, emphasis = false }) {
  const angle = `${slider.normalised * 270 - 135}deg`;
  const dragState = useRef(null);
  const valueText = formatValue(slider.control, slider.scaled);

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
        <span>{formatScaleValue(slider.control, slider.properties.start)}</span>
        <span>{formatScaleValue(slider.control, slider.properties.end)}</span>
      </div>
    </section>
  );
}

function ModeSelector({ combo }) {
  const current = Math.round(combo.choiceIndex);
  return (
    <div className="mode-selector" data-juce-control-parameter-index={combo.properties.parameterIndex} aria-label={combo.properties.name}>
      {combo.properties.choices.map((choice, index) => (
        <button type="button" key={choice} className={index === current ? "is-active" : ""} aria-pressed={index === current} onClick={() => combo.setChoiceIndex(index)}>
          {choice}
        </button>
      ))}
    </div>
  );
}

function ToggleButton({ toggle }) {
  return (
    <button
      type="button"
      className={`suite-toggle ${toggle.value ? "is-on" : ""}`}
      aria-pressed={toggle.value}
      data-juce-control-parameter-index={toggle.properties.parameterIndex}
      onClick={() => toggle.setValue(!toggle.value)}
    >
      <span>{toggle.properties.name}</span>
      <strong>{toggle.value ? "On" : "Off"}</strong>
    </button>
  );
}

export function SuitePluginApp({ product }) {
  useControlParameterIndexBridge();
  const sliders = Object.fromEntries(product.controls.map((control) => [control.id, useSliderRelay(control)]));
  const mode = useComboRelay("mode", product.modes);
  const toggles = product.toggles.map((toggle) => useToggleRelay(toggle));
  const levels = useLevels();
  const modeName = mode.properties.choices[Math.round(mode.choiceIndex)] || product.modes[0];
  const primary = sliders[product.primary] || Object.values(sliders)[0];
  const stripSliders = Object.values(sliders).filter((slider) => slider.id !== primary.id);

  return (
    <main
      className={`plugin-shell plugin-${product.slug}`}
      style={{
        "--accent-a": product.accents[0],
        "--accent-b": product.accents[1],
        "--accent-c": product.accents[2]
      }}
    >
      <header className="top-bar">
        <ProductMark product={product} />
        <ModeSelector combo={mode} />
        <div className="toggle-bank">
          {toggles.length > 0 ? toggles.map((toggle) => <ToggleButton key={toggle.id} toggle={toggle} />) : <div className="suite-badge">VST3</div>}
        </div>
      </header>

      <section className="workbench">
        <div className="meter-bank">
          <Meter label="In" value={levels.input} />
          <Meter label="Out" value={levels.output} />
        </div>

        <div className="curve-panel">
          <div className="curve-heading">
            <span>{modeName}</span>
            <strong>{product.description}</strong>
          </div>
          <Visualizer product={product} sliders={sliders} modeName={modeName} levels={levels} />
        </div>

        <KnobControl slider={primary} emphasis />
      </section>

      <section className="knob-strip" aria-label={`${product.name} controls`}>
        {stripSliders.map((slider) => <KnobControl key={slider.id} slider={slider} />)}
      </section>
    </main>
  );
}
