// app/api/v1/health/route.ts
import { NextResponse } from "next/server";

export async function GET() {
  return NextResponse.json({
    ok: true,
    message: "API alive",
    routes: [
      "/api/v1/health",
      "/api/v1/bootstrap",
      "/api/v1/device/next",
      "/api/v1/device/ack",
      "/api/v1/payments/generate",
    ],
  });
}
