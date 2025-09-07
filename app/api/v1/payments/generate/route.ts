// app/api/v1/payments/generate/route.ts
import { NextResponse } from "next/server";
import { prisma } from "../../../../../src/lib/prisma";
import { buildPaymentUri, NdefJobPayload } from "../../../../../src/lib/ndef";

type Body = {
  amount: number;
  currency?: string;
  label?: string;
  deviceKey?: string; // which writer to send the job to
  autoWrite?: boolean;
};

export async function POST(req: Request) {
  try {
    const data = (await req.json()) as Body;
    if (!data || typeof data.amount !== "number") {
      return NextResponse.json({ ok: false, error: "amount is required (number)" }, { status: 400 });
    }

    const currency = data.currency || "PKR";
    const payment = await prisma.payment.create({
      data: { amount: Math.round(data.amount), currency, label: data.label ?? null, status: "PENDING" },
    });

    const base = process.env.BASE_URL;
    if (!base) {
      return NextResponse.json({ ok: false, error: "BASE_URL not set" }, { status: 500 });
    }

    const nfcUri = buildPaymentUri(base, payment.id);

    // persist URI and mark ready
    await prisma.payment.update({ where: { id: payment.id }, data: { nfcUri, status: "READY" } });

    let jobId: string | undefined = undefined;
    if (data.autoWrite !== false) {
      const deviceKey = data.deviceKey || process.env.DEVICE_KEY;
      if (!deviceKey) {
        return NextResponse.json({ ok: false, error: "No deviceKey available for autowrite" }, { status: 400 });
      }
      const device = await prisma.device.findUnique({ where: { key: deviceKey } });
      if (!device) {
        return NextResponse.json({ ok: false, error: "Device not registered yet. Call /api/v1/bootstrap from the ESP32 first." }, { status: 400 });
      }

      const payload: NdefJobPayload = { uri: nfcUri };
      const job = await prisma.tagWriteJob.create({
        data: {
          deviceId: device.id,
          type: "NDEF_URI",
          payload,
          status: "QUEUED",
          paymentId: payment.id,
        },
      });
      jobId = job.id;
    }

    return NextResponse.json({ ok: true, paymentId: payment.id, nfcUri, jobId });
  } catch (err: any) {
    console.error("generate payment error", err);
    return NextResponse.json({ ok: false, error: "Internal error" }, { status: 500 });
  }
}
