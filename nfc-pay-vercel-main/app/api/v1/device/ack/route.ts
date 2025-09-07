import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";

export const runtime = "nodejs";

/**
 * Device ACK stub.
 * Accepts { jobId?, sessionId?, ok? } and returns 200.
 * No TagWriteJob model required; this route is a no-op for compatibility.
 */
export async function POST(req: NextRequest) {
  const body = await req.json().catch(() => ({}));
  const sessionId = String(body.sessionId || body.jobId || "");
  if (sessionId) {
    // Optional soft touch: try to find a session, but ignore errors
    await prisma.session.findUnique({ where: { id: sessionId } }).catch(() => null);
  }
  return NextResponse.json({ ok: true });
}
