// app/api/v1/bootstrap/route.ts
import { NextResponse } from "next/server";
import { prisma } from "../../../../src/lib/prisma";

export async function GET(req: Request) {
  try {
    const url = new URL(req.url);
    const deviceKey = req.headers.get("x-device-key") || url.searchParams.get("deviceKey") || process.env.DEVICE_KEY;
    const deviceName = req.headers.get("x-device-name") || url.searchParams.get("deviceName") || "ESP32 Writer";

    if (!deviceKey) {
      return NextResponse.json({ ok: false, error: "Missing deviceKey" }, { status: 400 });
    }

    const device = await prisma.device.upsert({
      where: { key: deviceKey },
      create: { key: deviceKey, name: deviceName },
      update: { name: deviceName },
    });

    // Touch lastSeen via a write op (updatedAt runs on update)
    await prisma.device.update({ where: { id: device.id }, data: { name: device.name } });

    return NextResponse.json({
      ok: true,
      deviceId: device.id,
      deviceKey: device.key,
      baseUrl: process.env.BASE_URL,
      message: "Bootstrap OK",
    });
  } catch (err: any) {
    console.error("bootstrap error", err);
    return NextResponse.json({ ok: false, error: "Internal error" }, { status: 500 });
  }
}
