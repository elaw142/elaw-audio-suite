export const product = {
  "name": "Drift",
  "slug": "drift",
  "tagline": "delay movement field",
  "description": "Modulated delay and widening effect",
  "primary": "time",
  "accents": [
    "#5eead4",
    "#93c5fd",
    "#f9a8d4"
  ],
  "visual": "drift",
  "controls": [
    {
      "id": "time",
      "name": "Time",
      "label": "ms",
      "format": "ms",
      "start": 1,
      "end": 800,
      "normalised": 0.18,
      "scaled": 42
    },
    {
      "id": "feedback",
      "name": "Feedback",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 95,
      "normalised": 0.28,
      "scaled": 27
    },
    {
      "id": "rate",
      "name": "Rate",
      "label": "Hz",
      "format": "hz1",
      "start": 0.05,
      "end": 8,
      "normalised": 0.18,
      "scaled": 0.75
    },
    {
      "id": "depth",
      "name": "Depth",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 100,
      "normalised": 0.42,
      "scaled": 42
    },
    {
      "id": "spread",
      "name": "Spread",
      "label": "%",
      "format": "signedPercent",
      "start": -100,
      "end": 100,
      "normalised": 0.68,
      "scaled": 36
    },
    {
      "id": "tone",
      "name": "Tone",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 100,
      "normalised": 0.62,
      "scaled": 62
    },
    {
      "id": "mix",
      "name": "Mix",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 100,
      "normalised": 1,
      "scaled": 100
    },
    {
      "id": "output",
      "name": "Output",
      "label": "dB",
      "format": "db",
      "start": -24,
      "end": 12,
      "normalised": 0.67,
      "scaled": 0
    }
  ],
  "modes": [
    "Chorus",
    "Flange",
    "Dub",
    "Wide"
  ],
  "toggles": []
};
