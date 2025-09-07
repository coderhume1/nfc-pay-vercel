import { prisma } from "@/lib/prisma";
import { isAdminAuthed } from "@/lib/auth";

export const dynamic = "force-dynamic";
export const runtime = "nodejs";


async function getPending(terminalId: string){
  const rows = await prisma.session.findMany({
    where: { terminalId, status: "pending" },
    orderBy: { createdAt: "desc" },
    take: 20,
  });
  return rows;
}

async function PendingList({ terminalId }: { terminalId: string }){
  const rows = await getPending(terminalId);
  if (!rows.length) return <p className="text-sm text-gray-500 mt-2">No pending sessions.</p>;
  const newestId = rows[0].id;
  return (
    <div className="mt-3 overflow-x-auto">
      <table className="w-full text-sm">
        <thead>
          <tr><th className="text-left p-2">ID</th><th className="text-left p-2">Created</th><th className="text-left p-2">Actions</th></tr>
        </thead>
        <tbody>
          {rows.map(r => (
            <tr key={r.id} className="border-t">
              <td className="p-2">{r.id}</td>
              <td className="p-2">{r.createdAt.toISOString()}</td>
              <td className="p-2">
                {r.id === newestId ? <span className="badge">Newest</span> : (
                  <form method="post" action="/api/admin/sessions/cancel">
                    <input type="hidden" name="sessionId" value={r.id} />
                    <input type="hidden" name="terminalId" value={terminalId} />
                    <button className="btn-outline">Cancel</button>
                  </form>
                )}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    
      {/* Pending Session Manager */}
      {isAdmin && (
        <div className="card">
          <h2 className="text-lg font-semibold">Pending Session Manager</h2>
          <p className="text-sm text-gray-600">Only the most recent pending can be approved. Cancel older ones here.</p>
          <PendingList terminalId={terminalId} />
          <form method="post" action="/api/admin/sessions/cancel-older" className="mt-3">
            <input type="hidden" name="terminalId" value={terminalId} />
            <button className="btn-outline">Cancel All Older Pending</button>
          </form>
        </div>
      )}

    </div>
  );
}


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
    
      {/* Pending Session Manager */}
      {isAdmin && (
        <div className="card">
          <h2 className="text-lg font-semibold">Pending Session Manager</h2>
          <p className="text-sm text-gray-600">Only the most recent pending can be approved. Cancel older ones here.</p>
          <PendingList terminalId={terminalId} />
          <form method="post" action="/api/admin/sessions/cancel-older" className="mt-3">
            <input type="hidden" name="terminalId" value={terminalId} />
            <button className="btn-outline">Cancel All Older Pending</button>
          </form>
        </div>
      )}

    </div>
  );
}
