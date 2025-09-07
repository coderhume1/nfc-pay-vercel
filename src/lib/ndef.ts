// src/lib/ndef.ts
export function buildPaymentUri(baseUrl: string, paymentId: string) {
  const url = new URL(baseUrl);
  // Link pattern: https://host/pay/<paymentId>
  url.pathname = `/pay/${paymentId}`;
  return url.toString();
}

export type NdefJobPayload = {
  uri: string;
  // For future extension: text?: string; mime?: string; etc.
};
