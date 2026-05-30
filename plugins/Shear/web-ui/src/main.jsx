import React from "react";
import { createRoot } from "react-dom/client";
import { SuitePluginApp } from "@elaw/ui/SuitePluginApp.jsx";
import "@elaw/ui/styles.css";
import { product } from "./product.js";

createRoot(document.getElementById("root")).render(
  <React.StrictMode>
    <SuitePluginApp product={product} />
  </React.StrictMode>
);
