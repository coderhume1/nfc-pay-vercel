import { prisma } from "@/lib/prisma";

export const dynamic = "force-dynamic";
export const runtime = "nodejs";

export default async function Page({ params }: { params: { terminalId: string } }) {
  const { terminalId } = params;
  const session = await prisma.session.findFirst({
    where: { terminalId, status: "pending" },
    orderBy: { createdAt: "desc" },
  });
  return (
    <div className="max-w-xl mx-auto">
      <div className="card">
        <h1 className="text-xl font-semibold">Checkout — {terminalId}</h1>
        {session ? (
          <>
            <div className="mt-4 grid grid-cols-2 gap-3">
              <div>
                <div className="text-xs text-gray-500">Session</div>
                <div className="font-mono text-sm break-all">{session.id}</div>
              </div>
              <div>
                <div className="text-xs text-gray-500">Status</div>
                <div>
                  <span className="badge badge-pending">pending</span>
                </div>
              </div>
              <div>
                <div className="text-xs text-gray-500">Amount</div>
                <div className="text-lg font-semibold">{session.amount} {session.currency}</div>
              </div>
              <div>
                <div className="text-xs text-gray-500">Created</div>
                <div className="text-sm">{session.createdAt.toISOString()}</div>
              </div>
            </div>
            <form className="mt-6" method="post" action="/api/sandbox/pay">
              <input type="hidden" name="sessionId" value={session.id} />
              <button className="btn w-full" type="submit">Approve (Sandbox)</button>
            </form>
          </>
        ) : (
          <p className="text-gray-600 mt-2">No pending session for this terminal.</p>
        )}
      </div>
    </div>
  );
}
