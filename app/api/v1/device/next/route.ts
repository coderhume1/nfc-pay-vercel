// app/api/v1/device/next/route.ts
import { NextResponse } from "next/server";
import { prisma } from "../../../../../src/lib/prisma";

export async function GET(req: Request) {
  try {
    const url = new URL(req.url);
    const key = req.headers.get("x-device-key") || url.searchParams.get("deviceKey") || process.env.DEVICE_KEY;
    if (!key) return NextResponse.json({ ok: false, error: "Missing deviceKey" }, { status: 400 });

    const device = await prisma.device.findUnique({ where: { key } });
    if (!device) return NextResponse.json({ ok: true, noJob: true }, { status: 204 });

    const job = await prisma.tagWriteJob.findFirst({
      where: { deviceId: device.id, status: "QUEUED" },
      orderBy: { createdAt: "asc" },
    });

    if (!job) return NextResponse.json({ ok: true, noJob: true }, { status: 204 });

    // mark as SENT
    await prisma.tagWriteJob.update({ where: { id: job.id }, data: { status: "SENT" } });

    return NextResponse.json({
      ok: true,
      job: {
        id: job.id,
        type: job.type,
        payload: job.payload,
      },
    });
  } catch (err: any) {
    console.error("device/next error", err);
    return NextResponse.json({ ok: false, error: "Internal error" }, { status: 500 });
  }
}
