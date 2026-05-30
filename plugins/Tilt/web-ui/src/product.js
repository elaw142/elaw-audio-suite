export const product = {
  "name": "Tilt",
  "slug": "tilt",
  "tagline": "character tone utility",
  "description": "Tone tilt and character EQ",
  "primary": "tilt",
  "accents": [
    "#66d9ef",
    "#f0e46a",
    "#ff8a5b"
  ],
  "visual": "tilt",
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
      "id": "tilt",
      "name": "Tilt",
      "label": "%",
      "format": "signedPercent",
      "start": -100,
      "end": 100,
      "normalised": 0.5,
      "scaled": 0
    },
    {
      "id": "pivot",
      "name": "Pivot",
      "label": "Hz",
      "format": "hz",
      "start": 120,
      "end": 8000,
      "normalised": 0.43,
      "scaled": 1000
    },
    {
      "id": "lowCut",
      "name": "Low Cut",
      "label": "Hz",
      "format": "hz",
      "start": 20,
      "end": 420,
      "normalised": 0,
      "scaled": 20
    },
    {
      "id": "highCut",
      "name": "High Cut",
      "label": "Hz",
      "format": "hz",
      "start": 2500,
      "end": 20000,
      "normalised": 0.89,
      "scaled": 18000
    },
    {
      "id": "color",
      "name": "Color",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 100,
      "normalised": 0.35,
      "scaled": 35
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
    "Clean",
    "Warm",
    "Air",
    "Dark"
  ],
  "toggles": []
};
