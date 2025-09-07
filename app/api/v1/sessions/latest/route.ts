import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { requireApiKey } from "@/lib/auth";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

export async function GET(req: NextRequest) {
  const unauth = requireApiKey(req);
  if (unauth) return unauth;

  try {
    const { searchParams } = new URL(req.url);
    let terminalId = String(searchParams.get("terminalId") || "").trim();
    if (!terminalId) {
      const deviceId = (req.headers.get("x-device-id") || "").toUpperCase();
      if (deviceId) {
        const dev = await prisma.device.findUnique({ where: { deviceId } });
        if (dev) terminalId = dev.terminalId;
      }
    }
    if (!terminalId) {
      return NextResponse.json({ error: "terminalId required" }, { status: 400 });
    }

    const s = await prisma.session.findFirst({
      where: { terminalId, status: "pending" },
      orderBy: { createdAt: "desc" },
    });
    if (!s) return NextResponse.json({ error: "not_found" }, { status: 404 });
    return NextResponse.json(s);
  } catch (e: any) {
    const msg = (e && (e.message || String(e))) || "unknown";
    return NextResponse.json({ error: "server_error", message: msg }, { status: 500 });
  }
}
