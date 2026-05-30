export const product = {
  "name": "Shear",
  "slug": "shear",
  "tagline": "fracture-drive shaper",
  "description": "Waveform distortion shaper",
  "primary": "drive",
  "accents": [
    "#7df7ff",
    "#fff0a1",
    "#ff4b3f"
  ],
  "visual": "transfer",
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
      "id": "drive",
      "name": "Drive",
      "label": "x",
      "format": "x",
      "start": 1,
      "end": 28,
      "normalised": 0.26,
      "scaled": 5
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
      "id": "bias",
      "name": "Bias",
      "label": "%",
      "format": "signedPercent",
      "start": -50,
      "end": 50,
      "normalised": 0.5,
      "scaled": 0
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
    "Warm",
    "Hard",
    "Fold",
    "Crush"
  ],
  "toggles": [
    {
      "id": "hq",
      "name": "HQ"
    }
  ]
};
