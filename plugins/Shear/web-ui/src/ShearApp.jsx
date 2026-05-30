import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import * as Juce from "@elaw/ui/juce/index.js";
import { product } from "./product.js";

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
  return `${value.toFixed(0)}%`;
}

function formatScaleValue(control, value) {
  const format = control.format || "percent";
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
          input: clamp(0.28 + Math.sin(now * 1.7) * 0.12 + Math.sin(now * 9.1) * 0.035),
          output: clamp(0.42 + Math.sin(now * 2.3) * 0.18 + Math.sin(now * 11.4) * 0.045)
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
    <div className="shear-brand" aria-label="Shear by Elaw Audio">
      <svg className="shear-logo" viewBox="0 0 92 92" role="img" aria-label="Shear logo">
        <defs>
          <linearGradient id="shear-edge" x1="12" y1="8" x2="82" y2="84" gradientUnits="userSpaceOnUse">
            <stop offset="0" stopColor="#7df7ff" />
            <stop offset="0.48" stopColor="#fff0a1" />
            <stop offset="1" stopColor="#ff4b3f" />
          </linearGradient>
        </defs>
        <path className="logo-plate logo-plate-a" d="M13 26 57 8l22 18-44 18z" />
        <path className="logo-plate logo-plate-b" d="M14 64 58 46l20 17-44 18z" />
        <path className="logo-edge" d="M70 10 23 82" />
        <path className="logo-wave" d="M12 47h16l8-17 11 33 8-19h25" />
      </svg>
      <div className="shear-title">
        <span>Elaw Audio</span>
        <h1>Shear</h1>
      </div>
    </div>
  );
}

function dbLabel(value) {
  if (value <= 0.0001) return "-inf";
  return `${(20 * Math.log10(value)).toFixed(1)} dB`;
}

function LevelMeters({ levels }) {
  const input = clamp(levels.input) * 100;
  const output = clamp(levels.output) * 100;

  return (
    <div className="shear-meters" aria-label="Signal meters">
      <div className="meter-row">
        <span>In</span>
        <div className="blade-meter" aria-hidden="true">
          <i style={{ width: `${input}%` }} />
        </div>
        <strong>{dbLabel(levels.input)}</strong>
      </div>
      <div className="meter-row">
        <span>Out</span>
        <div className="blade-meter is-output" aria-hidden="true">
          <i style={{ width: `${output}%` }} />
        </div>
        <strong>{dbLabel(levels.output)}</strong>
      </div>
    </div>
  );
}

function HqToggle({ toggle }) {
  return (
    <button
      type="button"
      className={`hq-switch ${toggle.value ? "is-on" : ""}`}
      aria-pressed={toggle.value}
      data-juce-control-parameter-index={toggle.properties.parameterIndex}
      onClick={() => toggle.setValue(!toggle.value)}
    >
      <span>HQ</span>
      <strong>{toggle.value ? "On" : "Off"}</strong>
    </button>
  );
}

function ModeSelector({ combo }) {
  const current = Math.round(combo.choiceIndex);

  return (
    <div className="shear-mode-rack" data-juce-control-parameter-index={combo.properties.parameterIndex} aria-label={combo.properties.name}>
      {combo.properties.choices.map((choice, index) => (
        <button
          type="button"
          key={choice}
          className={index === current ? "is-active" : ""}
          aria-pressed={index === current}
          onClick={() => combo.setChoiceIndex(index)}
        >
          <span>{String(index + 1).padStart(2, "0")}</span>
          <strong>{choice}</strong>
        </button>
      ))}
    </div>
  );
}

function shearCurve(modeName, x, drive, bias) {
  const driven = x * drive + bias;
  if (modeName === "Hard") return clamp(driven * 0.72, -1, 1);
  if (modeName === "Fold") {
    const folded = 1 - Math.abs(((driven + 3) % 4) - 2);
    return clamp(folded * 2 - 1, -1, 1);
  }
  if (modeName === "Crush") return Math.round(Math.tanh(driven) * 16) / 16;
  return Math.tanh(driven * 1.08);
}

function TransferDisplay({ sliders, modeName, levels }) {
  const canvasRef = useRef(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const context = canvas.getContext("2d");
    let frame = 0;
    const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

    const draw = () => {
      const bounds = { width: canvas.clientWidth, height: canvas.clientHeight };
      const ratio = window.devicePixelRatio || 1;
      const backingWidth = Math.max(1, Math.round(bounds.width * ratio));
      const backingHeight = Math.max(1, Math.round(bounds.height * ratio));
      if (canvas.width !== backingWidth) canvas.width = backingWidth;
      if (canvas.height !== backingHeight) canvas.height = backingHeight;

      context.setTransform(ratio, 0, 0, ratio, 0, 0);
      context.clearRect(0, 0, bounds.width, bounds.height);

      const bg = context.createLinearGradient(0, 0, bounds.width, bounds.height);
      bg.addColorStop(0, "rgba(15, 26, 29, 0.96)");
      bg.addColorStop(0.5, "rgba(10, 10, 12, 0.98)");
      bg.addColorStop(1, "rgba(29, 12, 13, 0.96)");
      context.fillStyle = bg;
      context.fillRect(0, 0, bounds.width, bounds.height);

      context.strokeStyle = "rgba(244, 238, 217, 0.09)";
      context.lineWidth = 1;
      for (let x = 44; x < bounds.width; x += 44) {
        context.beginPath();
        context.moveTo(x, 0);
        context.lineTo(x + 58, bounds.height);
        context.stroke();
      }
      for (let y = 36; y < bounds.height; y += 36) {
        context.beginPath();
        context.moveTo(0, y);
        context.lineTo(bounds.width, y);
        context.stroke();
      }

      context.fillStyle = "rgba(255, 75, 63, 0.08)";
      context.beginPath();
      context.moveTo(bounds.width * 0.58, 0);
      context.lineTo(bounds.width, 0);
      context.lineTo(bounds.width, bounds.height);
      context.lineTo(bounds.width * 0.34, bounds.height);
      context.closePath();
      context.fill();

      const drive = sliders.drive?.scaled ?? 5;
      const bias = (sliders.bias?.scaled ?? 0) * 0.012;
      const tone = (sliders.tone?.normalised ?? 0.62);
      const mix = sliders.mix?.normalised ?? 1;
      const phase = performance.now() * 0.001;

      context.save();
      context.translate(0, Math.sin(phase * 1.1) * 3);
      context.strokeStyle = "rgba(125, 247, 255, 0.24)";
      context.lineWidth = 2;
      context.beginPath();
      for (let i = 0; i <= 180; i += 1) {
        const t = i / 180;
        const x = t * bounds.width;
        const wave = Math.sin(t * Math.PI * 4 + phase * 0.85) * (0.18 + levels.input * 0.12);
        const y = bounds.height * (0.5 - wave);
        if (i === 0) context.moveTo(x, y);
        else context.lineTo(x, y);
      }
      context.stroke();
      context.restore();

      const line = context.createLinearGradient(0, 0, bounds.width, 0);
      line.addColorStop(0, "#7df7ff");
      line.addColorStop(0.48, "#fff0a1");
      line.addColorStop(1, "#ff4b3f");

      context.shadowColor = "rgba(255, 75, 63, 0.55)";
      context.shadowBlur = 18;
      context.strokeStyle = line;
      context.lineWidth = 4;
      context.beginPath();
      for (let i = 0; i <= 260; i += 1) {
        const t = i / 260;
        const x = t * bounds.width;
        const centered = t * 2 - 1;
        const shaped = shearCurve(modeName, centered, drive, bias);
        const dryBlend = centered * (1 - mix) + shaped * mix;
        const y = bounds.height * (0.5 - dryBlend * (0.28 + tone * 0.15));
        if (i === 0) context.moveTo(x, y);
        else context.lineTo(x, y);
      }
      context.stroke();
      context.shadowBlur = 0;

      context.strokeStyle = "rgba(255, 240, 161, 0.7)";
      context.lineWidth = 1.5;
      context.beginPath();
      context.moveTo(bounds.width * 0.64, 12);
      context.lineTo(bounds.width * 0.42, bounds.height - 12);
      context.stroke();

      context.fillStyle = "rgba(255, 75, 63, 0.86)";
      const outputHeight = Math.max(8, bounds.height * levels.output * 0.76);
      context.fillRect(bounds.width - 20, bounds.height - 18 - outputHeight, 8, outputHeight);

      if (!reduceMotion) frame = window.requestAnimationFrame(draw);
    };

    draw();
    return () => {
      if (frame) window.cancelAnimationFrame(frame);
    };
  }, [levels.input, levels.output, modeName, sliders]);

  return <canvas className="transfer-canvas" ref={canvasRef} aria-label="Shear transfer display" />;
}

function Knob({ slider, variant = "standard" }) {
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
    const dragDelta = (startY - event.clientY) + ((event.clientX - startX) * 0.38);
    const sensitivity = event.shiftKey ? 540 : 220;
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
      className={`shear-knob is-${variant}`}
      style={{ "--angle": angle, "--value": slider.normalised }}
      data-juce-control-parameter-index={slider.properties.parameterIndex}
    >
      <div className="knob-label">
        <span>{slider.properties.name}</span>
        <strong>{valueText}</strong>
      </div>
      <div className="blade-knob-shell">
        <div className="blade-knob">
          <i />
          <b />
        </div>
        <div
          className="knob-touch"
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

function TrimFader({ slider }) {
  const valueText = formatValue(slider.control, slider.scaled);
  return (
    <section className="trim-fader" style={{ "--value": slider.normalised }} data-juce-control-parameter-index={slider.properties.parameterIndex}>
      <div>
        <span>{slider.properties.name}</span>
        <strong>{valueText}</strong>
      </div>
      <Knob slider={slider} variant="trim" />
    </section>
  );
}

export function ShearApp() {
  useControlParameterIndexBridge();
  const sliders = Object.fromEntries(product.controls.map((control) => [control.id, useSliderRelay(control)]));
  const mode = useComboRelay("mode", product.modes);
  const toggles = product.toggles.map((toggle) => useToggleRelay(toggle));
  const levels = useLevels();
  const modeName = mode.properties.choices[Math.round(mode.choiceIndex)] || product.modes[0];

  return (
    <main className={`shear-shell mode-${modeName.toLowerCase()}`}>
      <div className="shear-backdrop" aria-hidden="true" />
      <header className="shear-header">
        <ShearMark />
        <ModeSelector combo={mode} />
        <LevelMeters levels={levels} />
        {toggles.length > 0 ? <HqToggle toggle={toggles[0]} /> : <div className="hq-switch is-badge">VST3</div>}
      </header>

      <section className="shear-stage">
        <div className="drive-bay">
          <div className="drive-bay-top">
            <span>{modeName}</span>
            <strong>Drive Core</strong>
          </div>
          <Knob slider={sliders.drive} variant="drive" />
        </div>

        <div className="scope-bay">
          <div className="scope-header">
            <span>Transfer</span>
            <strong>{product.description}</strong>
          </div>
          <TransferDisplay sliders={sliders} modeName={modeName} levels={levels} />
          <div className="scope-footer" aria-hidden="true">
            <span>Dry</span>
            <span>Wet</span>
          </div>
        </div>

        <div className="trim-stack">
          <TrimFader slider={sliders.input} />
          <TrimFader slider={sliders.output} />
        </div>
      </section>

      <section className="shear-bottom-controls" aria-label="Shear shaping controls">
        <Knob slider={sliders.tone} variant="blade" />
        <Knob slider={sliders.bias} variant="blade" />
        <Knob slider={sliders.mix} variant="blade" />
      </section>
    </main>
  );
}
