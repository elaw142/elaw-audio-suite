export const product = {
  "name": "Clamp",
  "slug": "clamp",
  "tagline": "pressure and punch",
  "description": "Compressor and limiter with character",
  "primary": "threshold",
  "accents": [
    "#7dd3fc",
    "#c084fc",
    "#fb7185"
  ],
  "visual": "clamp",
  "controls": [
    {
      "id": "input",
      "name": "Input",
      "label": "dB",
      "format": "db",
      "start": -18,
      "end": 18,
      "normalised": 0.5,
      "scaled": 0
    },
    {
      "id": "threshold",
      "name": "Threshold",
      "label": "dB",
      "format": "db",
      "start": -48,
      "end": 0,
      "normalised": 0.54,
      "scaled": -22
    },
    {
      "id": "ratio",
      "name": "Ratio",
      "label": ":1",
      "format": "ratio",
      "start": 1,
      "end": 20,
      "normalised": 0.26,
      "scaled": 4
    },
    {
      "id": "attack",
      "name": "Attack",
      "label": "ms",
      "format": "ms",
      "start": 1,
      "end": 100,
      "normalised": 0.09,
      "scaled": 10
    },
    {
      "id": "release",
      "name": "Release",
      "label": "ms",
      "format": "ms",
      "start": 20,
      "end": 1000,
      "normalised": 0.24,
      "scaled": 250
    },
    {
      "id": "punch",
      "name": "Punch",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 100,
      "normalised": 0.38,
      "scaled": 38
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
    "Glue",
    "Snap",
    "Crush",
    "Limit"
  ],
  "toggles": [
    {
      "id": "autoGain",
      "name": "Auto"
    }
  ]
};
