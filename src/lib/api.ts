// src/lib/api.ts
export async function generatePayment(input: { amount: number; currency?: string; label?: string; autoWrite?: boolean }) {
  const res = await fetch("/api/v1/payments/generate", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(input),
  });
  if (!res.ok) throw new Error(`Failed: ${res.status}`);
  return res.json();
}
