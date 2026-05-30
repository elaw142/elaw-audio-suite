export const product = {
  "name": "Rift",
  "slug": "rift",
  "tagline": "grain fracture engine",
  "description": "Experimental grain and freeze effect",
  "primary": "size",
  "accents": [
    "#a3e635",
    "#38bdf8",
    "#f472b6"
  ],
  "visual": "rift",
  "controls": [
    {
      "id": "size",
      "name": "Size",
      "label": "ms",
      "format": "ms",
      "start": 10,
      "end": 500,
      "normalised": 0.32,
      "scaled": 165
    },
    {
      "id": "density",
      "name": "Density",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 100,
      "normalised": 0.48,
      "scaled": 48
    },
    {
      "id": "pitch",
      "name": "Pitch",
      "label": "st",
      "format": "semitone",
      "start": -12,
      "end": 12,
      "normalised": 0.5,
      "scaled": 0
    },
    {
      "id": "jitter",
      "name": "Jitter",
      "label": "%",
      "format": "percent",
      "start": 0,
      "end": 100,
      "normalised": 0.33,
      "scaled": 33
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
    "Grain",
    "Reverse",
    "Scatter",
    "Crush"
  ],
  "toggles": [
    {
      "id": "freeze",
      "name": "Freeze"
    }
  ]
};
