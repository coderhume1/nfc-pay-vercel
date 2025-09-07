import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
export const runtime = "nodejs";

export async function POST(req: NextRequest) {
  let sessionId = "";
  const ct = req.headers.get("content-type") || "";
  if (ct.includes("application/json")) {
    const body = await req.json().catch(()=>({}));
    sessionId = String(body.sessionId || "");
  } else {
    const form = await req.formData();
    sessionId = String(form.get("sessionId") || "");
  }
  if (!sessionId) return NextResponse.json({ error: "sessionId required" }, { status: 400 });
  const s = await prisma.session.update({ where: { id: sessionId }, data: { status: "paid" } });
  return NextResponse.redirect(new URL(`/p/${s.terminalId}`, req.url));
}
