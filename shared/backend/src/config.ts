export const config = {
  port:    Number(process.env.HACKPASS_PORT ?? "8443"),
  dataDir: process.env.HACKPASS_DATA_DIR ?? "./data",
  regenerateCert: process.env.HACKPASS_TLS_REGENERATE === "1",
} as const;
