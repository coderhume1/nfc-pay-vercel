import { prisma } from "@/lib/prisma";
import { isAdminAuthed } from "@/lib/auth";

export const dynamic = "force-dynamic";
export const runtime = "nodejs";

export default async function Page({ params }: { params: { terminalId: string } }) {
  const { terminalId } = params;
  const session = await prisma.session.findFirst({
    where: { terminalId, status: "pending" },
    orderBy: { createdAt: "desc" },
  });
  const isAdmin = isAdminAuthed();

  return (
    <div className="max-w-xl mx-auto space-y-4">
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

      {isAdmin && (
        <div className="card">
          <h2 className="text-lg font-semibold">Operator Tools</h2>
          <p className="text-sm text-gray-600">Generate a new payment for this terminal.</p>
          <form method="post" action="/api/admin/sessions/create" className="grid grid-cols-1 md:grid-cols-5 gap-2 mt-3">
            <input name="terminalId" defaultValue={terminalId} className="border rounded-lg px-3 py-2 md:col-span-2" />
            <input name="amount" placeholder="Amount (e.g. 0 or 500)" className="border rounded-lg px-3 py-2 md:col-span-1" />
            <input name="currency" placeholder="USD" className="border rounded-lg px-3 py-2 md:col-span-1" />
            <button className="btn md:col-span-1" type="submit">Generate Payment</button>
          </form>
          <p className="text-xs text-gray-500 mt-2">Leave amount empty to use the default for the device/store.</p>
        </div>
      )}
    </div>
  );
}
