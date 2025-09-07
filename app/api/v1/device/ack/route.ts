// app/api/v1/device/ack/route.ts
import { NextResponse } from "next/server";
import { prisma } from "../../../../../src/lib/prisma";

type Body = {
  jobId: string;
  ok: boolean;
  error?: string;
};

export async function POST(req: Request) {
  try {
    const body = (await req.json()) as Body;
    if (!body?.jobId || typeof body.ok !== "boolean") {
      return NextResponse.json({ ok: false, error: "jobId and ok are required" }, { status: 400 });
    }

    const job = await prisma.tagWriteJob.update({
      where: { id: body.jobId },
      data: { status: body.ok ? "ACK_OK" : "ACK_ERR" },
      include: { payment: true },
    });

    if (job.paymentId) {
      await prisma.payment.update({
        where: { id: job.paymentId },
        data: { status: body.ok ? "WRITTEN" : "FAILED" },
      });
    }

    return NextResponse.json({ ok: true });
  } catch (err: any) {
    console.error("device/ack error", err);
    return NextResponse.json({ ok: false, error: "Internal error" }, { status: 500 });
  }
}
